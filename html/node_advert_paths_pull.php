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

try
{
    checkAuth();

    if ($_SERVER['REQUEST_METHOD'] !== 'GET')
    {
        jsonResponse(
            [
                'success' => false,
                'error' => 'GET required'
            ],
            405
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

    $result = $db->query("
        SELECT
            public_key_hex,
            path_len,
            path_hash_size,
            path_text,
            DATE_FORMAT(last_seen_at, '%Y-%m-%d %H:%i:%s') AS last_seen_at
        FROM node_advert_paths
        WHERE public_key_hex IS NOT NULL
          AND public_key_hex <> ''
        ORDER BY public_key_hex ASC
    ");

    $paths = [];

    while ($row = $result->fetch_assoc())
    {
        $row['path_len'] = $row['path_len'] !== null ? (int)$row['path_len'] : null;
        $row['path_hash_size'] = $row['path_hash_size'] !== null ? (int)$row['path_hash_size'] : null;

        $paths[] = $row;
    }

    jsonResponse(
        [
            'success' => true,
            'count' => count($paths),
            'paths' => $paths
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
