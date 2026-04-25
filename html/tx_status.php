<?php
declare(strict_types=1);

header('Content-Type: application/json; charset=utf-8');

mysqli_report(MYSQLI_REPORT_ERROR | MYSQLI_REPORT_STRICT);

function normalizeOutboxError(string $lastError): string
{
    return trim(mb_strtolower($lastError, 'UTF-8'));
}

function isRoomPasswordRequired(string $lastError): bool
{
    return normalizeOutboxError($lastError) === 'room password required';
}

function isFailedOutboxError(string $lastError): bool
{
    $normalized = normalizeOutboxError($lastError);

    if ($normalized === '')
    {
        return false;
    }

    if (isRoomPasswordRequired($lastError))
    {
        return false;
    }

    return
        str_contains($normalized, 'send failed') ||
        str_contains($normalized, 'no confirm after retries');
}

function txStateLabel(int $status): string
{
    switch ($status)
    {
        case 0:
            return 'queued';

        case 1:
            return 'waiting_ack';

        case 2:
            return 'failed';

        default:
            return 'unknown';
    }
}

try
{
    $id = (int) ($_GET['id'] ?? 0);

    if ($id <= 0)
    {
        throw new RuntimeException('Ungültige ID.');
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

    $sql = "
        SELECT
            id,
            tx_kind,
            status,
            retry_count,
            current_ack_hex,
            target_name,
            room_name,
            room_node_id,
            channel_name,
            channel_idx,
            last_error
        FROM tx_outbox
        WHERE id = ?
        LIMIT 1
    ";
    $stmt = $db->prepare($sql);
    $stmt->bind_param('i', $id);
    $stmt->execute();

    $stmt->bind_result(
        $rowId,
        $txKind,
        $status,
        $retryCount,
        $ackHex,
        $targetName,
        $roomName,
        $roomNodeId,
        $channelName,
        $channelIdx,
        $lastError
    );

    $found = $stmt->fetch();
    $stmt->close();

    if (!$found)
    {
        $updateSql = "
            UPDATE chat_messages
            SET status = 1
            WHERE tx_outbox_id = ?
        ";

        $updateStmt = $db->prepare($updateSql);
        $updateStmt->bind_param('i', $id);
        $updateStmt->execute();
        $updateStmt->close();
        $db->close();

        echo json_encode(
            [
                'success' => true,
                'state' => 'missing',
                'id' => $id,
            ],
            JSON_UNESCAPED_UNICODE | JSON_UNESCAPED_SLASHES | JSON_PRETTY_PRINT
        );
        exit;
    }

    $lastError = (string) ($lastError ?? '');
    $retryCount = (int) ($retryCount ?? 0);
    $status = (int) ($status ?? 0);
    $ackHex = ($ackHex !== null && $ackHex !== '') ? (string) $ackHex : null;
    $targetName = ($targetName !== null) ? (string) $targetName : null;
    $roomName = ($roomName !== null) ? (string) $roomName : null;
    $roomNodeId = ($roomNodeId !== null) ? (int) $roomNodeId : null;
    $txKind = (int) ($txKind ?? 0);
    $channelName = ($channelName !== null) ? (string) $channelName : null;
    $channelIdx = ($channelIdx !== null) ? (int) $channelIdx : null;

    if (isRoomPasswordRequired($lastError))
    {
        $db->close();

        echo json_encode(
            [
                'success' => true,
                'state' => 'room_password_required',
                'id' => $id,
                'tx_kind' => $txKind,
                'status' => $status,
                'status_label' => txStateLabel($status),
                'retry_count' => $retryCount,
                'current_ack_hex' => $ackHex,
                'target_name' => $targetName,
                'room_name' => $roomName,
                'room_node_id' => $roomNodeId,
                'last_error' => $lastError,
                'channel_name' => $channelName,
                'channel_idx' => $channelIdx,
            ],
            JSON_UNESCAPED_UNICODE | JSON_UNESCAPED_SLASHES | JSON_PRETTY_PRINT
        );
        exit;
    }

    if (isFailedOutboxError($lastError) || $status === 2)
    {
        $updateSql = "
            UPDATE chat_messages
            SET status = 2
            WHERE tx_outbox_id = ?
        ";

        $updateStmt = $db->prepare($updateSql);
        $updateStmt->bind_param('i', $id);
        $updateStmt->execute();
        $updateStmt->close();

        $deleteSql = "DELETE FROM tx_outbox WHERE id = ? LIMIT 1";
        $deleteStmt = $db->prepare($deleteSql);
        $deleteStmt->bind_param('i', $id);
        $deleteStmt->execute();
        $deleteStmt->close();
        $db->close();

        echo json_encode(
            [
                'success' => true,
                'state' => 'failed',
                'id' => $id,
                'tx_kind' => $txKind,
                'status' => $status,
                'status_label' => txStateLabel($status),
                'retry_count' => $retryCount,
                'current_ack_hex' => $ackHex,
                'target_name' => $targetName,
                'room_name' => $roomName,
                'room_node_id' => $roomNodeId,
                'last_error' => $lastError,
                'deleted' => true,
            ],
            JSON_UNESCAPED_UNICODE | JSON_UNESCAPED_SLASHES | JSON_PRETTY_PRINT
        );
        exit;
    }
    $db->close();

    echo json_encode(
        [
            'success' => true,
            'state' => 'pending',
            'id' => $id,
            'tx_kind' => $txKind,
            'status' => $status,
            'status_label' => txStateLabel($status),
            'retry_count' => $retryCount,
            'current_ack_hex' => $ackHex,
            'target_name' => $targetName,
            'room_name' => $roomName,
            'room_node_id' => $roomNodeId,
            'last_error' => $lastError,
        ],
        JSON_UNESCAPED_UNICODE | JSON_UNESCAPED_SLASHES | JSON_PRETTY_PRINT
    );
}
catch (Throwable $e)
{
    http_response_code(500);

    echo json_encode(
        [
            'success' => false,
            'error' => $e->getMessage(),
        ],
        JSON_UNESCAPED_UNICODE | JSON_UNESCAPED_SLASHES | JSON_PRETTY_PRINT
    );
}
