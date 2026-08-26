<?php
declare(strict_types=1);

header('Content-Type: application/json; charset=utf-8');

mysqli_report(MYSQLI_REPORT_ERROR | MYSQLI_REPORT_STRICT);


function extractPubkeyPrefixesFromResultJson(?string $resultJson): array
{
    if ($resultJson === null || trim($resultJson) === '')
    {
        return [];
    }

    $payload = json_decode($resultJson, true);

    if (!is_array($payload) || !isset($payload['raw_hex']))
    {
        return [];
    }

    $rawHex = strtolower(preg_replace('/\s+/', '', (string) $payload['raw_hex']));

    if ($rawHex === '' || strlen($rawHex) % 2 !== 0 || !ctype_xdigit($rawHex))
    {
        return [];
    }

    $bytes = hex2bin($rawHex);

    if ($bytes === false || strlen($bytes) < 4)
    {
        return [];
    }

    $header = unpack('vtotal_count/vresult_count', substr($bytes, 0, 4));
    $resultCount = (int) ($header['result_count'] ?? 0);
    $recordSize = 9;
    $neededLength = 4 + ($resultCount * $recordSize);

    if ($resultCount <= 0 || strlen($bytes) < $neededLength)
    {
        return [];
    }

    $prefixes = [];
    $pos = 4;

    for ($i = 0; $i < $resultCount; $i++)
    {
        $prefixes[] = strtolower(bin2hex(substr($bytes, $pos, 4)));
        $pos += $recordSize;
    }

    return array_values(array_unique($prefixes));
}

function loadRepeaterNames(mysqli $db, array $pubkeyPrefixes): array
{
    if ($pubkeyPrefixes === [])
    {
        return [];
    }

    $placeholders = implode(',', array_fill(0, count($pubkeyPrefixes), '?'));
    $types = str_repeat('s', count($pubkeyPrefixes));

    $stmt = $db->prepare("\n        SELECT LOWER(LEFT(prefix6_hex, 8)) AS pubkey_prefix, name\n        FROM repeaternodes\n        WHERE LOWER(LEFT(prefix6_hex, 8)) IN ($placeholders)\n    ");
    $stmt->bind_param($types, ...$pubkeyPrefixes);
    $stmt->execute();
    $result = $stmt->get_result();

    $names = [];

    while ($row = $result->fetch_assoc())
    {
        $prefix = (string) $row['pubkey_prefix'];
        $name = $row['name'] !== null ? (string) $row['name'] : '';

        if ($prefix !== '' && $name !== '')
        {
            $names[$prefix] = $name;
        }
    }

    $stmt->close();

    return $names;
}

function jsonResponse(array $data, int $statusCode = 200): never
{
    http_response_code($statusCode);
    echo json_encode($data, JSON_UNESCAPED_UNICODE | JSON_UNESCAPED_SLASHES);
    exit;
}

try
{
    $db = new mysqli(
        'localhost',
        'meshcore',
        '',
        'meshcore',
        3306,
        '/run/mysqld/mysqld.sock'
    );

    $db->set_charset('utf8mb4');

    $command = (string) ($_GET['command'] ?? '');
    $data = [];

    if ($_SERVER['REQUEST_METHOD'] === 'POST')
    {
        $raw = file_get_contents('php://input');
        $data = $raw !== false && $raw !== ''
            ? json_decode($raw, true, 512, JSON_THROW_ON_ERROR)
            : [];
        $command = (string) ($data['command'] ?? $command);
    }

    if ($command === 'start')
    {
        $targetName = trim((string) ($data['target_name'] ?? ''));
        $authPassword = (string) ($data['auth_password'] ?? '');

        // Backward-compatible fallback for clients with a cached older
        // meshcore-api.js which sends only {command:"start"}.
        if ($targetName === '')
        {
            $stmt = $db->prepare('SELECT protected_repeater_name FROM companion_config WHERE id = 1 LIMIT 1');
            $stmt->execute();
            $result = $stmt->get_result();
            $row = $result->fetch_assoc();
            $stmt->close();

            if ($row !== null)
            {
                $targetName = trim((string) ($row['protected_repeater_name'] ?? ''));
            }
        }

        if ($targetName === '')
        {
            throw new RuntimeException('Kein Home Repeater angegeben.');
        }

        if (mb_strlen($targetName, 'UTF-8') > 64)
        {
            throw new RuntimeException('Home Repeater ist zu lang.');
        }

        if (strlen($authPassword) > 15)
        {
            throw new RuntimeException('Passwort darf maximal 15 Byte lang sein.');
        }

        // Only persist the Home Repeater name. Do NOT set apply_pending here:
        // a neighbour query must not reconfigure the radio or send a self advert.
        $stmt = $db->prepare('
            UPDATE companion_config
            SET protected_repeater_name = ?
            WHERE id = 1
        ');
        $stmt->bind_param('s', $targetName);
        $stmt->execute();

        if ($stmt->affected_rows === 0)
        {
            $check = $db->query('SELECT id FROM companion_config WHERE id = 1 LIMIT 1');
            if ($check->fetch_assoc() === null)
            {
                $stmt->close();
                throw new RuntimeException('Companion Setup ist nicht konfiguriert.');
            }
        }

        $stmt->close();

        $stmt = $db->prepare('
            INSERT INTO companion_actions
            (
                action_type,
                target_name,
                auth_password,
                status
            )
            VALUES
            (
                \'req_neighbours\',
                ?,
                NULLIF(?, \'\'),
                0
            )
        ');
        $stmt->bind_param('ss', $targetName, $authPassword);
        $stmt->execute();
        $actionId = $db->insert_id;
        $stmt->close();

        jsonResponse(
            [
                'success' => true,
                'action_id' => $actionId,
                'target_name' => $targetName
            ]
        );
    }

    if ($command === 'status')
    {
        $configResult = $db->query('
            SELECT protected_repeater_name
            FROM companion_config
            WHERE id = 1
            LIMIT 1
        ');
        $config = $configResult->fetch_assoc();

        $actionResult = $db->query('
            SELECT id, status, target_name, result_json, error_text, created_at, updated_at, processed_at
            FROM companion_actions
            WHERE action_type = \'req_neighbours\'
            ORDER BY id DESC
            LIMIT 1
        ');
        $action = $actionResult->fetch_assoc();
        $repeaterNames = $action
            ? loadRepeaterNames($db, extractPubkeyPrefixesFromResultJson($action['result_json'] !== null ? (string) $action['result_json'] : null))
            : [];

        jsonResponse(
            [
                'success' => true,
                'config' => [
                    'protected_repeater_name' => $config && $config['protected_repeater_name'] !== null
                        ? (string) $config['protected_repeater_name']
                        : ''
                ],
                'action' => $action ? [
                    'id' => (int) $action['id'],
                    'status' => (int) $action['status'],
                    'target_name' => $action['target_name'] !== null ? (string) $action['target_name'] : '',
                    'result_json' => $action['result_json'] !== null ? (string) $action['result_json'] : '',
                    'repeater_names' => $repeaterNames,
                    'error_text' => $action['error_text'] !== null ? (string) $action['error_text'] : '',
                    'created_at' => $action['created_at'],
                    'updated_at' => $action['updated_at'],
                    'processed_at' => $action['processed_at']
                ] : null
            ]
        );
    }

    throw new RuntimeException('Ungültiges Kommando.');
}
catch (Throwable $e)
{
    jsonResponse(
        [
            'success' => false,
            'error' => $e->getMessage()
        ],
        500
    );
}
