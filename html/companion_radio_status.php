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
            updated_at,
            json_text
        FROM companion_radio_status
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
                'status' => null
            ]
        );
    }

    $status = json_decode((string) $row['json_text'], true);

    if (!is_array($status))
    {
        throw new RuntimeException('Ungültiges JSON in companion_radio_status.json_text');
    }

    jsonResponse(
        [
            'success' => true,
            'status' =>
            [
                'updated_at' => $row['updated_at'],
                'noise_floor' => isset($status['noise_floor']) ? (float) $status['noise_floor'] : null,
                'last_rssi' => isset($status['last_rssi']) ? (float) $status['last_rssi'] : null,
                'last_snr' => isset($status['last_snr']) ? (float) $status['last_snr'] : null,
                'tx_air_secs' => isset($status['tx_air_secs']) ? (int) $status['tx_air_secs'] : null,
                'rx_air_secs' => isset($status['rx_air_secs']) ? (int) $status['rx_air_secs'] : null
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