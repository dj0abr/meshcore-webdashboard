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

    if (!is_array($json) || !isset($json['paths']) || !is_array($json['paths']))
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
        INSERT IGNORE INTO node_advert_paths
        (
            public_key_hex,
            path_len,
            path_hash_size,
            path_text,
            last_seen_at
        )
        VALUES
        (
            ?, ?, ?, ?, ?
        )
    ");

    $inserted = 0;
    $ignored = 0;
    $skipped = 0;

    $db->begin_transaction();

    foreach ($json['paths'] as $path)
    {
        if (!is_array($path))
        {
            $skipped++;
            continue;
        }

        $publicKeyHex = strtoupper(trim((string)($path['public_key_hex'] ?? '')));

        if ($publicKeyHex === '' || strlen($publicKeyHex) !== 64)
        {
            $skipped++;
            continue;
        }

        $pathLen = isset($path['path_len']) ? (int)$path['path_len'] : null;
        $pathHashSize = isset($path['path_hash_size']) ? (int)$path['path_hash_size'] : null;
        $pathText = nullIfEmpty($path['path_text'] ?? null);
        $lastSeenAt = nullIfEmpty($path['last_seen_at'] ?? null);

        if ($lastSeenAt === null)
        {
            $lastSeenAt = date('Y-m-d H:i:s');
        }

        $stmt->bind_param(
            'siiss',
            $publicKeyHex,
            $pathLen,
            $pathHashSize,
            $pathText,
            $lastSeenAt
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
