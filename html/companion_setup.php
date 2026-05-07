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
    $raw = file_get_contents('php://input');

    if ($raw === false || $raw === '')
    {
        throw new RuntimeException('Keine Eingabedaten erhalten.');
    }

    $data = json_decode($raw, true, 512, JSON_THROW_ON_ERROR);

    $name = trim((string) ($data['name'] ?? ''));
    $locationName = trim((string) ($data['location_name'] ?? ''));
    $latitude = (float) ($data['latitude'] ?? 0.0);
    $longitude = (float) ($data['longitude'] ?? 0.0);

    if ($name === '')
    {
        throw new RuntimeException('Name fehlt.');
    }

    if (mb_strlen($name, 'UTF-8') > 64)
    {
        throw new RuntimeException('Name ist zu lang.');
    }

    if (mb_strlen($locationName, 'UTF-8') > 128)
    {
        throw new RuntimeException('City ist zu lang.');
    }

    $locationNameDb = $locationName !== '' ? $locationName : null;

    if ($latitude < -90.0 || $latitude > 90.0)
    {
        throw new RuntimeException('Latitude ist ungültig.');
    }

    if ($longitude < -180.0 || $longitude > 180.0)
    {
        throw new RuntimeException('Longitude ist ungültig.');
    }

    $latitudeE6 = (int) round($latitude * 1000000.0);
    $longitudeE6 = (int) round($longitude * 1000000.0);

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
        INSERT INTO companion_config
        (
            id,
            name,
            latitude_e6,
            longitude_e6,
            location_name,
            radio_bw_hz,
            radio_sf,
            radio_cr,
            apply_pending,
            last_error
        )
        VALUES
        (
            1,
            ?,
            ?,
            ?,
            ?,
            62500,
            8,
            8,
            1,
            NULL
        )
        ON DUPLICATE KEY UPDATE
            name = VALUES(name),
            latitude_e6 = VALUES(latitude_e6),
            longitude_e6 = VALUES(longitude_e6),
            location_name = VALUES(location_name),
            radio_bw_hz = VALUES(radio_bw_hz),
            radio_sf = VALUES(radio_sf),
            radio_cr = VALUES(radio_cr),
            apply_pending = 1,
            last_error = NULL
    ";

    $stmt = $db->prepare($sql);
    $stmt->bind_param('siis', $name, $latitudeE6, $longitudeE6, $locationNameDb);
    $stmt->execute();
    $stmt->close();

    jsonResponse(
        [
            'success' => true
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