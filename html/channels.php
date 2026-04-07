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
            channel_idx,
            name,
            enabled,
            is_default,
            is_observed,
            has_local_context,
            sync_pending,
            sync_action,
            sync_error,
            UNIX_TIMESTAMP(last_seen_at) AS last_seen_epoch
        FROM channels
        WHERE NOT (sync_pending = 1 AND sync_action = 'delete')
        ORDER BY
            is_default DESC,
            enabled DESC,
            is_observed ASC,
            channel_idx ASC
    ";

    $result = $db->query($sql);

    $channels = [];

    while ($row = $result->fetch_assoc())
    {
        $channels[] =
        [
            'type' => 'channel',
            'channel_idx' => (int) $row['channel_idx'],
            'name' => (string) $row['name'],
            'enabled' => (int) $row['enabled'] === 1,
            'is_default' => (int) $row['is_default'] === 1,
            'is_observed' => (int) $row['is_observed'] === 1,
            'has_local_context' => (int) $row['has_local_context'] === 1,
            'sync_pending' => (int) $row['sync_pending'] === 1,
            'sync_action' => (string) $row['sync_action'],
            'sync_error' => (string) $row['sync_error'],
            'last_seen_epoch' => $row['last_seen_epoch'] !== null ? (int) $row['last_seen_epoch'] : null,
        ];
    }

    $db->close();

    jsonResponse(
        [
            'success' => true,
            'channels' => $channels
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