<?php
declare(strict_types=1);

header('Content-Type: application/json; charset=utf-8');

mysqli_report(MYSQLI_REPORT_ERROR | MYSQLI_REPORT_STRICT);

function jsonResponse(array $data, int $statusCode = 200): never
{
    http_response_code($statusCode);
    echo json_encode($data, JSON_UNESCAPED_UNICODE | JSON_UNESCAPED_SLASHES);
    exit;
}

function normalizeChannelName(string $name): string
{
    $name = trim($name);

    if ($name === '')
    {
        throw new RuntimeException('Channel-Name fehlt.');
    }

    if (mb_strlen($name, 'UTF-8') > 64)
    {
        throw new RuntimeException('Channel-Name ist zu lang.');
    }

    return $name;
}

function normalizeHexKey(string $hex): string
{
    $hex = strtoupper(trim($hex));
    $hex = preg_replace('/[^0-9A-F]/', '', $hex ?? '');

    if ($hex === null || $hex === '')
    {
        throw new RuntimeException('Secret Key fehlt.');
    }

    if (strlen($hex) !== 32)
    {
        throw new RuntimeException('Secret Key muss genau 32 Hex-Zeichen lang sein.');
    }

    return $hex;
}

function derivePublicKeyHex(string $name): string
{
    return strtoupper(substr(hash('sha256', $name), 0, 32));
}

function deriveHashtagKeyHex(string $name): string
{
    return strtoupper(substr(hash('sha256', $name), 0, 32));
}

function findChannelByNameAndKey(mysqli $db, string $name, string $keyHex): ?array
{
    $stmt = $db->prepare('
        SELECT channel_idx, name, join_mode, enabled, is_default, is_observed, has_local_context
        FROM channels
        WHERE name = ? AND key_hex = ?
        LIMIT 1
    ');
    $stmt->bind_param('ss', $name, $keyHex);
    $stmt->execute();

    $result = $stmt->get_result();
    $row = $result->fetch_assoc();
    $stmt->close();

    return $row ?: null;
}

function findChannelByIdx(mysqli $db, int $channelIdx): ?array
{
    $stmt = $db->prepare('
        SELECT channel_idx, name, join_mode, enabled, is_default, is_observed, has_local_context
        FROM channels
        WHERE channel_idx = ?
        LIMIT 1
    ');
    $stmt->bind_param('i', $channelIdx);
    $stmt->execute();

    $result = $stmt->get_result();
    $row = $result->fetch_assoc();
    $stmt->close();

    return $row ?: null;
}

function findNextFreeChannelIdx(mysqli $db, int $start = 1): int
{
    for ($idx = $start; $idx < 40; $idx++)
    {
        $stmt = $db->prepare('
            SELECT channel_idx
            FROM channels
            WHERE channel_idx = ?
              AND has_local_context = 1
            LIMIT 1
        ');
        $stmt->bind_param('i', $idx);
        $stmt->execute();
        $stmt->store_result();

        $exists = $stmt->num_rows > 0;
        $stmt->close();

        if (!$exists)
        {
            return $idx;
        }
    }

    throw new RuntimeException('Kein freier Channel-Slot mehr verfügbar.');
}

function upsertChannel(
    mysqli $db,
    int $channelIdx,
    string $name,
    int $joinMode,
    string $keyHex,
    bool $enabled,
    bool $isDefault
): void
{
    $existing = findChannelByIdx($db, $channelIdx);

    if ($existing !== null)
    {
        $sql = '
            UPDATE channels
            SET
                name = ?,
                join_mode = ?,
                key_hex = ?,
                enabled = ?,
                is_default = ?,
                has_local_context = 1,
                sync_pending = 1,
                sync_action = ?,
                sync_error = ?,
                last_seen_at = NOW()
            WHERE channel_idx = ?
            LIMIT 1
        ';

        $stmt = $db->prepare($sql);

        $enabledInt = $enabled ? 1 : 0;
        $isDefaultInt = $isDefault ? 1 : 0;
        $syncAction = 'upsert';
        $syncError = '';

        $stmt->bind_param(
            'sisiissi',
            $name,
            $joinMode,
            $keyHex,
            $enabledInt,
            $isDefaultInt,
            $syncAction,
            $syncError,
            $channelIdx
        );
        $stmt->execute();
        $stmt->close();
        return;
    }

    $sql = '
        INSERT INTO channels
        (
            channel_idx,
            name,
            join_mode,
            passphrase,
            key_hex,
            enabled,
            is_default,
            is_observed,
            has_local_context,
            sync_pending,
            sync_action,
            sync_error,
            last_seen_at
        )
        VALUES
        (
            ?, ?, ?, NULL, ?, ?, ?, 0, 1, 1, ?, ?, NOW()
        )
    ';

    $stmt = $db->prepare($sql);

    $enabledInt = $enabled ? 1 : 0;
    $isDefaultInt = $isDefault ? 1 : 0;
    $syncAction = 'upsert';
    $syncError = '';

    $stmt->bind_param(
        'isisiiss',
        $channelIdx,
        $name,
        $joinMode,
        $keyHex,
        $enabledInt,
        $isDefaultInt,
        $syncAction,
        $syncError
    );
    $stmt->execute();
    $stmt->close();
}

function ensurePublicChannel(mysqli $db): array
{
    $name = 'Public';
    $joinMode = 3;
    $keyHex = derivePublicKeyHex($name);
    $channelIdx = 0;

    upsertChannel(
        $db,
        $channelIdx,
        $name,
        $joinMode,
        $keyHex,
        true,
        true
    );

    return
    [
        'type' => 'channel',
        'channel_idx' => $channelIdx,
        'name' => $name,
        'enabled' => true,
        'is_default' => true,
        'is_observed' => false,
        'has_local_context' => true
    ];
}

try
{
    $raw = file_get_contents('php://input');

    if ($raw === false || $raw === '')
    {
        throw new RuntimeException('Keine Eingabedaten erhalten.');
    }

    $data = json_decode($raw, true, 512, JSON_THROW_ON_ERROR);

    $action = trim((string) ($data['action'] ?? ''));
    $name = trim((string) ($data['name'] ?? ''));
    $secretKey = trim((string) ($data['secret_key'] ?? ''));

    if ($action === '')
    {
        throw new RuntimeException('Keine Aktion angegeben.');
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

    $channelName = '';
    $joinMode = 0;
    $keyHex = '';
    $channelIdx = 0;
    $returnSecretKey = null;
    $isDefault = false;

    switch ($action)
    {
        case 'create_private':
        {
            $channelName = normalizeChannelName($name);
            $joinMode = 2;
            $keyHex = strtoupper(bin2hex(random_bytes(16)));
            $returnSecretKey = $keyHex;

            $existing = findChannelByNameAndKey($db, $channelName, $keyHex);

            if ($existing !== null)
            {
                $channelIdx = (int) $existing['channel_idx'];
            }
            else
            {
                $channelIdx = findNextFreeChannelIdx($db, 1);
            }

            break;
        }

        case 'join_private':
        {
            $channelName = normalizeChannelName($name);
            $joinMode = 2;
            $keyHex = normalizeHexKey($secretKey);

            $existing = findChannelByNameAndKey($db, $channelName, $keyHex);

            if ($existing !== null)
            {
                $channelIdx = (int) $existing['channel_idx'];
            }
            else
            {
                $channelIdx = findNextFreeChannelIdx($db, 1);
            }

            break;
        }

        case 'join_public':
        {
            $channelName = normalizeChannelName($name);
            $joinMode = 3;
            $keyHex = derivePublicKeyHex($channelName);

            if (strcasecmp($channelName, 'Public') === 0)
            {
                $public = ensurePublicChannel($db);

                jsonResponse(
                    [
                        'success' => true,
                        'channel' => $public,
                        'secret_key' => null
                    ]
                );
            }

            $existing = findChannelByNameAndKey($db, $channelName, $keyHex);

            if ($existing !== null)
            {
                $channelIdx = (int) $existing['channel_idx'];
            }
            else
            {
                $channelIdx = findNextFreeChannelIdx($db, 1);
            }

            break;
        }

        case 'join_hashtag':
        {
            $channelName = normalizeChannelName($name);

            if ($channelName[0] !== '#')
            {
                $channelName = '#' . $channelName;
            }

            $joinMode = 1;
            $keyHex = deriveHashtagKeyHex($channelName);

            $existing = findChannelByNameAndKey($db, $channelName, $keyHex);

            if ($existing !== null)
            {
                $channelIdx = (int) $existing['channel_idx'];
            }
            else
            {
                $channelIdx = findNextFreeChannelIdx($db, 1);
            }

            break;
        }

        default:
            throw new RuntimeException('Unbekannte Aktion.');
    }

    upsertChannel(
        $db,
        $channelIdx,
        $channelName,
        $joinMode,
        $keyHex,
        true,
        $isDefault
    );

    jsonResponse(
        [
            'success' => true,
            'channel' =>
            [
                'type' => 'channel',
                'channel_idx' => $channelIdx,
                'name' => $channelName,
                'enabled' => true,
                'is_default' => $isDefault,
                'is_observed' => false,
                'has_local_context' => true
            ],
            'secret_key' => $returnSecretKey
        ]
    );
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