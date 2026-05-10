<?php
declare(strict_types=1);

header('Content-Type: application/json; charset=utf-8');

mysqli_report(MYSQLI_REPORT_ERROR | MYSQLI_REPORT_STRICT);

const API_TOKEN = '99877849829572943099528987283483784';

function jsonResponse(array $data, int $statusCode = 200): never
{
    http_response_code($statusCode);
    echo json_encode($data, JSON_UNESCAPED_UNICODE | JSON_UNESCAPED_SLASHES);
    exit;
}

function checkAuth(): void
{
    $token = $_SERVER['HTTP_X_API_TOKEN'] ?? '';

    if ($token !== API_TOKEN)
    {
        jsonResponse(
            [
                'success' => false,
                'error' => 'Unauthorized'
            ],
            401
        );
    }
}

function nullIfEmpty(mixed $value): mixed
{
    if ($value === '')
    {
        return null;
    }

    return $value;
}

try
{
    checkAuth();

    if ($_SERVER['REQUEST_METHOD'] !== 'POST')
    {
        jsonResponse(
            [
                'success' => false,
                'error' => 'POST required'
            ],
            405
        );
    }

    $raw = file_get_contents('php://input');
    $json = json_decode($raw, true);

    if (!is_array($json) || !isset($json['nodes']) || !is_array($json['nodes']))
    {
        jsonResponse(
            [
                'success' => false,
                'error' => 'Invalid JSON payload'
            ],
            400
        );
    }

    $db = new mysqli(
        'localhost',
        'meshcore',
        '',
        'meshcore',
        3306,
        '/run/mysqld/mysqld.sock'
    );

    $db->set_charset('utf8mb4');

    $stmt = $db->prepare("
        INSERT IGNORE INTO repeaternodes
        (
            node_id,
            advert_type,
            advert_flags,
            name,
            public_key_hex,
            prefix6_hex,
            adv_lat_e6,
            adv_lon_e6,
            last_advert_at,
            last_mod_at,
            first_seen_at
        )
        VALUES
        (
            ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?
        )
    ");

    $inserted = 0;
    $ignored = 0;
    $skipped = 0;

    $db->begin_transaction();

    foreach ($json['nodes'] as $node)
    {
        if (!is_array($node))
        {
            $skipped++;
            continue;
        }

        $publicKeyHex = strtoupper(trim((string)($node['public_key_hex'] ?? '')));

        if ($publicKeyHex === '' || strlen($publicKeyHex) !== 64)
        {
            $skipped++;
            continue;
        }

        $nodeId = isset($node['node_id']) ? (int)$node['node_id'] : null;
        $advertType = isset($node['advert_type']) ? (int)$node['advert_type'] : 0;
        $advertFlags = isset($node['advert_flags']) ? (int)$node['advert_flags'] : 0;
        $name = (string)($node['name'] ?? '');
        $prefix6Hex = nullIfEmpty($node['prefix6_hex'] ?? null);
        $advLatE6 = isset($node['adv_lat_e6']) ? (int)$node['adv_lat_e6'] : null;
        $advLonE6 = isset($node['adv_lon_e6']) ? (int)$node['adv_lon_e6'] : null;
        $lastAdvertAt = nullIfEmpty($node['last_advert_at'] ?? null);
        $lastModAt = nullIfEmpty($node['last_mod_at'] ?? null);
        $firstSeenAt = nullIfEmpty($node['first_seen_at'] ?? null);

        if ($firstSeenAt === null)
        {
            $firstSeenAt = date('Y-m-d H:i:s');
        }

        $stmt->bind_param(
            'iiisssiisss',
            $nodeId,
            $advertType,
            $advertFlags,
            $name,
            $publicKeyHex,
            $prefix6Hex,
            $advLatE6,
            $advLonE6,
            $lastAdvertAt,
            $lastModAt,
            $firstSeenAt
        );

        $stmt->execute();

        if ($stmt->affected_rows > 0)
        {
            $inserted++;
        }
        else
        {
            $ignored++;
        }
    }

    $db->commit();

    jsonResponse(
        [
            'success' => true,
            'inserted' => $inserted,
            'ignored' => $ignored,
            'skipped' => $skipped
        ]
    );
}
catch (Throwable $e)
{
    if (isset($db) && $db instanceof mysqli)
    {
        try
        {
            $db->rollback();
        }
        catch (Throwable)
        {
        }
    }

    jsonResponse(
        [
            'success' => false,
            'error' => $e->getMessage()
        ],
        500
    );
}