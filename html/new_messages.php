<?php

declare(strict_types=1);

header('Content-Type: application/json; charset=utf-8');

mysqli_report(MYSQLI_REPORT_ERROR | MYSQLI_REPORT_STRICT);

try
{
    $afterId = (int) ($_GET['after_id'] ?? 0);

    if ($afterId < 0)
    {
        $afterId = 0;
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
        SELECT
            id,
            timestamp_epoch,
            chat_kind,
            name,
            room_sender_name,
            channel_idx,
            channel_key_hex,
            `text`,
            sender_prefix6_hex,
            snr_db,
            path_len
        FROM chat_messages
        WHERE direction = 0
        AND id > ?
        ORDER BY id ASC
    ');

    $stmt->bind_param('i', $afterId);
    $stmt->execute();

    $stmt->bind_result(
        $id,
        $timestampEpoch,
        $chatKind,
        $name,
        $roomSenderName,
        $channelIdx,
        $channelKeyHex,
        $text,
        $senderPrefix6Hex,
        $snrDb,
        $pathLen
    );

    $messages = [];
    $newestId = $afterId;

    while ($stmt->fetch())
    {
        $idValue = (int) $id;
        $newestId = max($newestId, $idValue);

        $messages[] =
        [
            'id' => $idValue,
            'timestamp_epoch' => (int) $timestampEpoch,
            'chat_kind' => (int) $chatKind,
            'name' => (string) $name,
            'room_sender_name' => $roomSenderName !== null ? (string) $roomSenderName : null,
            'channel_idx' => $channelIdx !== null ? (int) $channelIdx : null,
            'channel_key_hex' => $channelKeyHex !== null ? (string) $channelKeyHex : null,
            'text' => (string) $text,
            'sender_prefix6_hex' => $senderPrefix6Hex !== null ? (string) $senderPrefix6Hex : null,
            'snr_db' => $snrDb !== null ? (float) $snrDb : null,
            'path_len' => $pathLen !== null ? (int) $pathLen : null,
        ];
    }

    $stmt->close();
    $db->close();

    echo json_encode(
        [
            'success' => true,
            'after_id' => $afterId,
            'newest_id' => $newestId,
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