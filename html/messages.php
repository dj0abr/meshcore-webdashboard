<?php
declare(strict_types=1);

header('Content-Type: application/json; charset=utf-8');

mysqli_report(MYSQLI_REPORT_ERROR | MYSQLI_REPORT_STRICT);

function normalizeOutboxError(string $lastError): string
{
    return trim(mb_strtolower($lastError, 'UTF-8'));
}

function buildUiState(
    int $direction,
    int $chatKind,
    int $chatStatus,
    ?int $txStatus,
    string $lastError
): string
{
    if ($direction !== 1)
    {
        return '';
    }

    if ($chatStatus === 1)
    {
        return 'confirmed';
    }

    if ($chatStatus === 2)
    {
        return 'failed';
    }

    $normalized = normalizeOutboxError($lastError);

    if ($normalized === 'room password required')
    {
        return 'room_password_required';
    }

    if ($normalized === 'another room login pending')
    {
        return 'room_login_blocked';
    }

    if ($normalized === 'room login pending')
    {
        return 'room_login_pending';
    }

    if ($normalized === 'room login timeout')
    {
        return 'room_login_timeout';
    }

    if ($normalized === 'room login failed')
    {
        return 'room_login_failed';
    }

    if ($normalized === 'room login command failed')
    {
        return 'room_login_command_failed';
    }

    if ($normalized === 'room not found')
    {
        return 'room_not_found';
    }

    if ($txStatus === null)
    {
        return 'confirmed';
    }

    if ($txStatus === 1)
    {
        return 'waiting_ack';
    }

    if ($txStatus === 2)
    {
        return 'failed';
    }

    if ($txStatus === 3)
    {
        return 'confirmed';
    }

    if ($txStatus === 0)
    {
        if ($chatKind === 1)
        {
            return 'room_preparing';
        }

        return 'queued';
    }

    if ($chatKind === 1)
    {
        return 'room_pending';
    }

    return 'pending';
}

function buildUiStatusText(
    string $uiState,
    int $retryCount,
    string $lastError
): string
{
    switch ($uiState)
    {
        case '':
            return '';

        case 'confirmed':
            return 'Bestätigt';

        case 'failed':
            if ($lastError !== '')
            {
                return 'Fehlgeschlagen: ' . $lastError;
            }

            return 'Fehlgeschlagen';

        case 'waiting_ack':
            return 'Warte auf Bestätigung, Retry: ' . $retryCount;

        case 'room_password_required':
            return 'Room-Passwort erforderlich';

        case 'room_login_blocked':
            return 'Anderer Room-Login läuft';

        case 'room_login_pending':
            return 'Room-Login läuft';

        case 'room_login_timeout':
            return 'Room antwortet nicht, Retry: ' . $retryCount;

        case 'room_login_failed':
            return 'Room-Login fehlgeschlagen';

        case 'room_login_command_failed':
            return 'Room-Login konnte nicht gestartet werden';

        case 'room_not_found':
            return 'Room nicht gefunden';

        case 'room_preparing':
            return 'Bereite Room vor';

        case 'room_pending':
            return 'Room wird vorbereitet';

        case 'queued':
            return 'In Warteschlange';

        default:
            return 'Pending';
    }
}

try
{
    $name = trim((string) ($_GET['name'] ?? ''));
    $kind = trim((string) ($_GET['kind'] ?? ''));
    $channelKeyHex = strtoupper(trim((string) ($_GET['channel_key_hex'] ?? '')));

    if ($kind !== 'dm' && $kind !== 'room' && $kind !== 'channel')
    {
        throw new RuntimeException('Ungültiger Chat-Typ. Erlaubt sind nur dm, room oder channel.');
    }

    if ($kind === 'channel')
    {
        if ($channelKeyHex === '' || !preg_match('/^[0-9A-F]{32}$/', $channelKeyHex))
        {
            throw new RuntimeException('Ungültige channel_key_hex.');
        }

        $chatKind = 2;
    }
    else
    {
        if ($name === '')
        {
            throw new RuntimeException('Kein Chatname angegeben.');
        }

        $chatKind = ($kind === 'room') ? 1 : 0;
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

    if ($kind === 'channel')
    {
        $sql = "
            SELECT *
            FROM
            (
                SELECT
                    cm.id,
                    cm.timestamp_epoch,
                    cm.direction,
                    cm.chat_kind,
                    cm.name,
                    cm.room_sender_name,
                    cm.channel_idx,
                    cm.channel_key_hex,
                    cm.`text`,
                    cm.status,
                    cm.tx_outbox_id,
                    cm.sender_prefix6_hex,
                    cm.snr_db,
                    cm.path_len,
                    cm.correlation_key,
                    tx.status AS tx_status,
                    tx.retry_count,
                    tx.last_error,
                    tx.room_node_id,
                    tx.room_name,
                    tx.target_name,
                    tx.channel_name,
                    tx.channel_idx AS tx_channel_idx,
                    tx.channel_key_hex AS tx_channel_key_hex
                FROM chat_messages cm
                LEFT JOIN tx_outbox tx
                    ON tx.id = cm.tx_outbox_id
                WHERE cm.chat_kind = 2
                AND cm.channel_key_hex = ?
                ORDER BY cm.timestamp_epoch DESC, cm.id DESC
                LIMIT 1000
            ) AS recent
            ORDER BY recent.timestamp_epoch ASC, recent.id ASC
        ";

        $stmt = $db->prepare($sql);
        $stmt->bind_param('s', $channelKeyHex);
    }
    else
    {
        $sql = "
            SELECT *
            FROM
            (
                SELECT
                    cm.id,
                    cm.timestamp_epoch,
                    cm.direction,
                    cm.chat_kind,
                    cm.name,
                    cm.room_sender_name,
                    cm.channel_idx,
                    cm.channel_key_hex,
                    cm.`text`,
                    cm.status,
                    cm.tx_outbox_id,
                    cm.sender_prefix6_hex,
                    cm.snr_db,
                    cm.path_len,
                    cm.correlation_key,
                    tx.status AS tx_status,
                    tx.retry_count,
                    tx.last_error,
                    tx.room_node_id,
                    tx.room_name,
                    tx.target_name,
                    tx.channel_name,
                    tx.channel_idx AS tx_channel_idx,
                    tx.channel_key_hex AS tx_channel_key_hex
                FROM chat_messages cm
                LEFT JOIN tx_outbox tx
                    ON tx.id = cm.tx_outbox_id
                WHERE cm.chat_kind = ?
                AND cm.name = ?
                ORDER BY cm.timestamp_epoch DESC, cm.id DESC
                LIMIT 1000
            ) AS recent
            ORDER BY recent.timestamp_epoch ASC, recent.id ASC
        ";

        $stmt = $db->prepare($sql);
        $stmt->bind_param('is', $chatKind, $name);
    }

    $stmt->execute();

    $stmt->bind_result(
        $id,
        $timestampEpoch,
        $direction,
        $chatKindDb,
        $nameDb,
        $roomSenderName,
        $channelIdxDb,
        $channelKeyHexDb,
        $text,
        $status,
        $txOutboxId,
        $senderPrefix6Hex,
        $snrDb,
        $pathLen,
        $correlationKey,
        $txStatus,
        $retryCount,
        $lastError,
        $roomNodeId,
        $roomName,
        $targetName,
        $txChannelName,
        $txChannelIdx,
        $txChannelKeyHex
    );

    $messages = [];

    while ($stmt->fetch())
    {
        $directionValue = (int) $direction;
        $chatKindValue = (int) $chatKindDb;
        $statusValue = (int) $status;
        $txStatusValue = ($txStatus !== null) ? (int) $txStatus : null;
        $retryCountValue = ($retryCount !== null) ? (int) $retryCount : 0;
        $lastErrorValue = trim((string) ($lastError ?? ''));

        $uiState = buildUiState(
            $directionValue,
            $chatKindValue,
            $statusValue,
            $txStatusValue,
            $lastErrorValue
        );

        $messages[] =
        [
            'id' => (int) $id,
            'timestamp_epoch' => (int) $timestampEpoch,
            'direction' => $directionValue,
            'chat_kind' => $chatKindValue,
            'name' => (string) $nameDb,
            'room_sender_name' => ($roomSenderName !== null) ? (string) $roomSenderName : null,
            'text' => (string) $text,
            'status' => $statusValue,
            'tx_outbox_id' => ($txOutboxId !== null) ? (int) $txOutboxId : null,
            'sender_prefix6_hex' => $senderPrefix6Hex,
            'snr_db' => ($snrDb !== null) ? (float) $snrDb : null,
            'path_len' => ($pathLen !== null) ? (int) $pathLen : null,
            'correlation_key' => ($correlationKey !== null) ? (string) $correlationKey : null,
            'tx_status' => $txStatusValue,
            'retry_count' => $retryCountValue,
            'last_error' => $lastErrorValue,
            'room_node_id' => ($roomNodeId !== null) ? (int) $roomNodeId : null,
            'room_name' => ($roomName !== null) ? (string) $roomName : null,
            'target_name' => ($targetName !== null) ? (string) $targetName : null,
            'ui_state' => $uiState,
            'ui_status_text' => buildUiStatusText($uiState, $retryCountValue, $lastErrorValue),
            'channel_idx' => $channelIdxDb !== null ? (int) $channelIdxDb : null,
            'channel_key_hex' => $channelKeyHexDb !== null ? (string) $channelKeyHexDb : null,
            'channel_name' => $txChannelName !== null ? (string) $txChannelName : null,
            'tx_channel_idx' => $txChannelIdx !== null ? (int) $txChannelIdx : null,
            'tx_channel_key_hex' => $txChannelKeyHex !== null ? (string) $txChannelKeyHex : null,
        ];
    }

    $stmt->close();
    $db->close();

    echo json_encode(
        [
            'success' => true,
            'name' => $name,
            'channel_key_hex' => $kind === 'channel' ? $channelKeyHex : null,
            'kind' => $kind,
            'messages' => $messages,
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