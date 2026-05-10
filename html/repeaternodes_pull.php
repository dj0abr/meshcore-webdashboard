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
            node_id,
            advert_type,
            advert_flags,
            name,
            public_key_hex,
            prefix6_hex,
            adv_lat_e6,
            adv_lon_e6,
            DATE_FORMAT(last_advert_at, '%Y-%m-%d %H:%i:%s') AS last_advert_at,
            DATE_FORMAT(last_mod_at, '%Y-%m-%d %H:%i:%s') AS last_mod_at,
            DATE_FORMAT(first_seen_at, '%Y-%m-%d %H:%i:%s') AS first_seen_at
        FROM repeaternodes
        WHERE public_key_hex IS NOT NULL
          AND public_key_hex <> ''
        ORDER BY id ASC
    ");

    $nodes = [];

    while ($row = $result->fetch_assoc())
    {
        $row['node_id'] = $row['node_id'] !== null ? (int)$row['node_id'] : null;
        $row['advert_type'] = (int)$row['advert_type'];
        $row['advert_flags'] = (int)$row['advert_flags'];
        $row['adv_lat_e6'] = $row['adv_lat_e6'] !== null ? (int)$row['adv_lat_e6'] : null;
        $row['adv_lon_e6'] = $row['adv_lon_e6'] !== null ? (int)$row['adv_lon_e6'] : null;

        $nodes[] = $row;
    }

    jsonResponse(
        [
            'success' => true,
            'count' => count($nodes),
            'nodes' => $nodes
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