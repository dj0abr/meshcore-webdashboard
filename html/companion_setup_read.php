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

    $sql = "
        SELECT
            name,
            location_name,
            protected_repeater_name,
            latitude_e6,
            longitude_e6,
            bot,
            apply_pending,
            last_applied_at,
            last_error,
            updated_at
        FROM companion_config
        WHERE id = 1
        LIMIT 1
    ";

    $result = $db->query($sql);
    $row = $result->fetch_assoc();

    if (!$row)
    {
        jsonResponse(
            [
                'success' => true,
                'config' => null
            ]
        );
    }

    $latitude = ((int) $row['latitude_e6']) / 1000000.0;
    $longitude = ((int) $row['longitude_e6']) / 1000000.0;

    jsonResponse(
        [
            'success' => true,
            'config' =>
            [
                'name' => (string) $row['name'],
                'location_name' => $row['location_name'] !== null ? (string) $row['location_name'] : '',
                'protected_repeater_name' => $row['protected_repeater_name'] !== null ? (string) $row['protected_repeater_name'] : '',
                'latitude' => $latitude,
                'longitude' => $longitude,
                'bot' => ((int) $row['bot'] === 1),
                'apply_pending' => ((int) $row['apply_pending'] === 1),
                'last_applied_at' => $row['last_applied_at'],
                'last_error' => $row['last_error'],
                'updated_at' => $row['updated_at']
            ]
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