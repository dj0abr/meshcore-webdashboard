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
            c.channel_idx,
            c.key_hex,
            c.name,
            c.enabled,
            c.is_default,
            c.is_observed,
            c.has_local_context,
            c.sync_pending,
            c.sync_action,
            c.sync_error,
            UNIX_TIMESTAMP(c.last_seen_at) AS last_seen_epoch,
            COALESCE(cm.message_count, 0) AS message_count
        FROM channels c
        LEFT JOIN
        (
            SELECT
                channel_key_hex,
                COUNT(*) AS message_count
            FROM chat_messages
            WHERE chat_kind = 2
              AND channel_key_hex IS NOT NULL
            GROUP BY channel_key_hex
        ) cm
            ON cm.channel_key_hex = c.key_hex
        WHERE NOT (c.sync_pending = 1 AND c.sync_action = 'delete')
        ORDER BY
            c.is_default DESC,
            c.enabled DESC,
            c.is_observed ASC,
            c.channel_idx ASC
    ";
    $result = $db->query($sql);

    $channels = [];

    while ($row = $result->fetch_assoc())
    {
        $channels[] =
        [
            'type' => 'channel',
            'channel_idx' => (int) $row['channel_idx'],
            'key_hex' => isset($row['key_hex']) ? (string) $row['key_hex'] : '',
            'name' => (string) $row['name'],
            'enabled' => (int) $row['enabled'] === 1,
            'is_default' => (int) $row['is_default'] === 1,
            'is_observed' => (int) $row['is_observed'] === 1,
            'has_local_context' => (int) $row['has_local_context'] === 1,
            'sync_pending' => (int) $row['sync_pending'] === 1,
            'sync_action' => (string) $row['sync_action'],
            'sync_error' => (string) $row['sync_error'],
            'last_seen_epoch' => $row['last_seen_epoch'] !== null ? (int) $row['last_seen_epoch'] : null,
            'message_count' => isset($row['message_count']) ? (int) $row['message_count'] : 0,
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