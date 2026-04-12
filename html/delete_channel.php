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
    $keyHex = strtoupper(trim((string) ($data['key_hex'] ?? '')));

    if ($keyHex === '' || !preg_match('/^[0-9A-F]{32}$/', $keyHex))
    {
        throw new RuntimeException('Ungültige key_hex.');
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

    $stmt = $db->prepare('
        SELECT is_default, has_local_context
        FROM channels
        WHERE key_hex = ?
        LIMIT 1
    ');
    $stmt->bind_param('s', $keyHex);
    $stmt->execute();
    $stmt->bind_result($isDefault, $hasLocalContext);

    $found = $stmt->fetch();
    $stmt->close();

    if (!$found)
    {
        throw new RuntimeException('Channel nicht gefunden.');
    }

    if ((int) $isDefault === 1)
    {
        throw new RuntimeException('Der Default Channel kann nicht gelöscht werden.');
    }

    if ((int) $hasLocalContext !== 1)
    {
        $stmt = $db->prepare('
            DELETE FROM channels
            WHERE key_hex = ?
              AND has_local_context = 0
            LIMIT 1
        ');
        $stmt->bind_param('s', $keyHex);
        $stmt->execute();
        $deletedRows = $stmt->affected_rows;
        $stmt->close();

        $db->close();

        jsonResponse(
            [
                'success' => true,
                'key_hex' => $keyHex,
                'delete_requested' => false,
                'deleted_local_record' => $deletedRows > 0
            ]
        );
    }

    $syncAction = 'delete';
    $syncError = '';

    $stmt = $db->prepare('
        UPDATE channels
        SET
            sync_pending = 1,
            sync_action = ?,
            sync_error = ?
        WHERE key_hex = ?
        LIMIT 1
    ');
    $stmt->bind_param('sss', $syncAction, $syncError, $keyHex);
    $stmt->execute();
    $updatedRows = $stmt->affected_rows;
    $stmt->close();

    $db->close();

    jsonResponse(
        [
            'success' => true,
            'key_hex' => $keyHex,
            'delete_requested' => $updatedRows > 0
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