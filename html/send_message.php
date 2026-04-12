<?php

declare(strict_types=1);

header('Content-Type: application/json; charset=utf-8');

mysqli_report(MYSQLI_REPORT_ERROR | MYSQLI_REPORT_STRICT);

try
{
    $raw = file_get_contents('php://input');

    if ($raw === false)
    {
        throw new RuntimeException('Request konnte nicht gelesen werden.');
    }

    $data = json_decode($raw, true);

    if (!is_array($data))
    {
        throw new RuntimeException('Ungültiges JSON im Request: ' . $raw);
    }

    $txKind = (int) ($data['tx_kind'] ?? 0);
    $targetName = trim((string) ($data['target_name'] ?? ''));
    $targetNodeId = (int) ($data['target_node_id'] ?? 0);
    $roomName = trim((string) ($data['room_name'] ?? ''));
    $roomNodeId = (int) ($data['room_node_id'] ?? 0);
    $messageText = trim((string) ($data['message_text'] ?? ''));
    $maxRetries = (int) ($data['max_retries'] ?? 3);
    $channelName = trim((string) ($data['channel_name'] ?? ''));
    $channelKeyHex = strtoupper(trim((string) ($data['channel_key_hex'] ?? '')));
    $channelIdx = null;

    if ($txKind !== 0 && $txKind !== 1 && $txKind !== 2 && $txKind !== 3)
    {
        throw new RuntimeException('Ungültiger tx_kind. Erlaubt sind nur 0, 1, 2 oder 3.');
    }

    if ($messageText === '')
    {
        throw new RuntimeException('Leere Nachricht ist nicht erlaubt.');
    }

    if (mb_strlen($messageText, 'UTF-8') > 400)
    {
        throw new RuntimeException('Nachricht ist zu lang.');
    }

    if ($maxRetries < 0)
    {
        $maxRetries = 0;
    }

    if ($maxRetries > 20)
    {
        $maxRetries = 20;
    }

    if ($txKind === 0)
    {
        if ($targetName === '')
        {
            throw new RuntimeException('Kein Zielname angegeben.');
        }

        if (mb_strlen($targetName, 'UTF-8') > 64)
        {
            throw new RuntimeException('Zielname ist zu lang.');
        }
    }
    else if ($txKind === 1)
    {
        if ($roomName === '')
        {
            throw new RuntimeException('Kein Room-Name angegeben.');
        }

        if (mb_strlen($roomName, 'UTF-8') > 64)
        {
            throw new RuntimeException('Room-Name ist zu lang.');
        }
    }
    else if ($txKind === 2)
    {
        // FloodAdvert braucht weder target_name noch room_name
    }
    else if ($txKind === 3)
    {
        if ($channelKeyHex === '' || !preg_match('/^[0-9A-F]{32}$/', $channelKeyHex))
        {
            throw new RuntimeException('Ungültige channel_key_hex.');
        }

        if ($channelName !== '' && mb_strlen($channelName, 'UTF-8') > 64)
        {
            throw new RuntimeException('Channel-Name ist zu lang.');
        }
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
    $db->begin_transaction();

    if ($txKind === 3)
    {
        $channelStmt = $db->prepare('
            SELECT channel_idx, name, enabled
            FROM channels
            WHERE key_hex = ?
            LIMIT 1
        ');
        $channelStmt->bind_param('s', $channelKeyHex);
        $channelStmt->execute();
        $channelResult = $channelStmt->get_result();
        $channelRow = $channelResult->fetch_assoc();
        $channelStmt->close();

        if (!$channelRow)
        {
            throw new RuntimeException('Channel mit dieser channel_key_hex wurde nicht gefunden.');
        }

        if ((int) $channelRow['enabled'] !== 1)
        {
            throw new RuntimeException('Channel ist deaktiviert.');
        }

        $channelIdx = (int) $channelRow['channel_idx'];

        if ($channelName === '')
        {
            $channelName = trim((string) ($channelRow['name'] ?? ''));
        }
    }

    $sql = "
        INSERT INTO tx_outbox
        (
            tx_kind,
            target_name,
            target_node_id,
            room_name,
            room_node_id,
            channel_name,
            channel_idx,
            channel_key_hex,
            message_text,
            status,
            retry_count,
            max_retries,
            next_attempt_epoch
        )
        VALUES
        (
            ?,
            NULLIF(?, ''),
            ?,
            NULLIF(?, ''),
            ?,
            NULLIF(?, ''),
            ?,
            NULLIF(?, ''),
            ?,
            0,
            0,
            ?,
            UNIX_TIMESTAMP()
        )
    ";

    $stmt = $db->prepare($sql);
    $stmt->bind_param(
        'isisisissi',
        $txKind,
        $targetName,
        $targetNodeId,
        $roomName,
        $roomNodeId,
        $channelName,
        $channelIdx,
        $channelKeyHex,
        $messageText,
        $maxRetries
    );
    $stmt->execute();

    $txOutboxId = (int) $db->insert_id;
    $chatMessageId = 0;

    if ($txKind === 0 || $txKind === 1 || $txKind === 3)
    {
        if ($txKind === 0)
        {
            $chatKind = 0;
            $chatName = $targetName;
            $chatChannelIdx = null;
            $chatChannelKeyHex = null;
        }
        else if ($txKind === 1)
        {
            $chatKind = 1;
            $chatName = $roomName;
            $chatChannelIdx = null;
            $chatChannelKeyHex = null;
        }
        else
        {
            $chatKind = 2;
            $chatName = ($channelName !== '') ? $channelName : ('ch' . $channelIdx);
            $chatChannelIdx = $channelIdx;
            $chatChannelKeyHex = $channelKeyHex;
        }

        $chatSql = "
            INSERT INTO chat_messages
            (
                timestamp_epoch,
                direction,
                chat_kind,
                name,
                channel_idx,
                channel_key_hex,
                `text`,
                status,
                tx_outbox_id
            )
            VALUES
            (
                UNIX_TIMESTAMP(),
                1,
                ?,
                ?,
                ?,
                ?,
                ?,
                0,
                ?
            )
        ";

        $chatStmt = $db->prepare($chatSql);
        $chatStmt->bind_param(
            'isissi',
            $chatKind,
            $chatName,
            $chatChannelIdx,
            $chatChannelKeyHex,
            $messageText,
            $txOutboxId
        );
        $chatStmt->execute();

        $chatMessageId = (int) $db->insert_id;
    }

    $db->commit();

    echo json_encode(
        [
            'success' => true,
            'id' => $txOutboxId,
            'chat_message_id' => $chatMessageId,
            'tx_kind' => $txKind,
            'target_name' => $targetName,
            'target_node_id' => $targetNodeId,
            'room_name' => $roomName,
            'room_node_id' => $roomNodeId,
            'message_text' => $messageText,
            'channel_name' => $channelName,
            'channel_idx' => $channelIdx,
            'channel_key_hex' => ($channelKeyHex !== '') ? $channelKeyHex : null,
        ],
        JSON_UNESCAPED_UNICODE | JSON_UNESCAPED_SLASHES
    );
}
catch (Throwable $e)
{
    if (isset($db) && ($db instanceof mysqli))
    {
        try
        {
            $db->rollback();
        }
        catch (Throwable $ignored)
        {
        }
    }

    http_response_code(500);

    echo json_encode(
        [
            'success' => false,
            'error' => $e->getMessage(),
            'type' => get_class($e),
            'file' => basename($e->getFile()),
            'line' => $e->getLine(),
        ],
        JSON_UNESCAPED_UNICODE | JSON_UNESCAPED_SLASHES
    );
}
