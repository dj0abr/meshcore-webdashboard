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

function jobStatusText(int $status): string
{
    switch ($status)
    {
        case 0:
            return 'queued';
        case 1:
            return 'running';
        case 2:
            return 'failed';
        case 3:
            return 'done';
        case 4:
            return 'skipped';
        default:
            return 'unknown';
    }
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

    $jobResult = $db->query("
        SELECT
            id,
            created_at,
            started_at,
            finished_at,
            status,
            type_filter,
            requested_by,
            result_count,
            error_text
        FROM discover_jobs
        ORDER BY id DESC
        LIMIT 1
    ");

    $jobRow = $jobResult->fetch_assoc();

    $job = null;
    $results = [];

    if ($jobRow)
    {
        $jobId = (int) $jobRow['id'];

        $job =
        [
            'id' => $jobId,
            'created_at' => $jobRow['created_at'],
            'started_at' => $jobRow['started_at'],
            'finished_at' => $jobRow['finished_at'],
            'status' => (int) $jobRow['status'],
            'status_text' => jobStatusText((int) $jobRow['status']),
            'type_filter' => (int) $jobRow['type_filter'],
            'requested_by' => $jobRow['requested_by'],
            'result_count' => (int) $jobRow['result_count'],
            'error_text' => $jobRow['error_text'],
        ];

        $resultsQuery = $db->query("
            SELECT
                r.node_id_hex,
                COALESCE(n.name, r.node_id_hex) AS node_name,
                r.snr_rx_db,
                r.snr_tx_db,
                r.rssi_dbm,
                r.source_code,
                r.updated_at
            FROM discover_results r
            LEFT JOIN
            (
                SELECT n0.public_key_hex, n0.name
                FROM nodes n0
                WHERE n0.public_key_hex IS NOT NULL
                AND NOT EXISTS
                (
                    SELECT 1
                    FROM repeaternodes r0
                    WHERE r0.public_key_hex IS NOT NULL
                    AND LOWER(r0.public_key_hex) = LOWER(n0.public_key_hex)
                )
                UNION ALL
                SELECT public_key_hex, name FROM repeaternodes
                WHERE public_key_hex IS NOT NULL
            ) n
                ON LEFT(n.public_key_hex, 16) = r.node_id_hex
            WHERE r.last_job_id = {$jobId}
            ORDER BY r.rssi_dbm DESC, r.node_id_hex ASC
        ");

        while ($row = $resultsQuery->fetch_assoc())
        {
            $results[] =
            [
                'node_id_hex' => $row['node_id_hex'],
                'node_name' => $row['node_name'],
                'snr_rx_db' => (float) $row['snr_rx_db'],
                'snr_tx_db' => (float) $row['snr_tx_db'],
                'rssi_dbm' => (int) $row['rssi_dbm'],
                'source_code' => (int) $row['source_code'],
                'updated_at' => $row['updated_at'],
            ];
        }
    }
    jsonResponse(
        [
            'success' => true,
            'job' => $job,
            'results' => $results,
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