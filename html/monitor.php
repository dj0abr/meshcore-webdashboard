<?php
declare(strict_types=1);

header('Content-Type: application/json; charset=utf-8');

mysqli_report(MYSQLI_REPORT_ERROR | MYSQLI_REPORT_STRICT);

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

    $result = $db->query('
        SELECT
            id,
            created_at,
            source,
            push_code,
            payload_len,
            payload_hex,
            packet_valid,
            decode_error,
            snr_db,
            rssi_dbm,
            route_type,
            payload_type,
            payload_version,
            path_len,
            path_hash_size,
            path_text,
            pkt_hash,
            rf_packet_hex,
            pkt_payload_len,
            pkt_payload_hex,
            req_valid,
            req_dst_hash,
            req_src_hash,
            req_mac,
            req_cipher_len,
            grp_txt_valid,
            grp_channel_hash,
            grp_mac,
            grp_decrypt_tried,
            grp_decrypt_ok,
            grp_mac_verified,
            grp_timestamp,
            grp_txt_type,
            grp_channel_name,
            grp_channel_key_hex,
            grp_text,
            advert_valid,
            advert_public_key_hex,
            advert_timestamp,
            advert_flags,
            advert_role,
            advert_has_gps,
            advert_has_ble,
            advert_has_shortcut,
            advert_has_name,
            advert_latitude_e6,
            advert_longitude_e6,
            advert_name
        FROM (
            SELECT
                id,
                created_at,
                source,
                push_code,
                payload_len,
                payload_hex,
                packet_valid,
                decode_error,
                snr_db,
                rssi_dbm,
                route_type,
                payload_type,
                payload_version,
                path_len,
                path_hash_size,
                path_text,
                pkt_hash,
                rf_packet_hex,
                pkt_payload_len,
                pkt_payload_hex,
                req_valid,
                req_dst_hash,
                req_src_hash,
                req_mac,
                req_cipher_len,
                grp_txt_valid,
                grp_channel_hash,
                grp_mac,
                grp_decrypt_tried,
                grp_decrypt_ok,
                grp_mac_verified,
                grp_timestamp,
                grp_txt_type,
                grp_channel_name,
                grp_channel_key_hex,
                grp_text,
                advert_valid,
                advert_public_key_hex,
                advert_timestamp,
                advert_flags,
                advert_role,
                advert_has_gps,
                advert_has_ble,
                advert_has_shortcut,
                advert_has_name,
                advert_latitude_e6,
                advert_longitude_e6,
                advert_name
            FROM meshcore_monitor
            ORDER BY id DESC
            LIMIT 1000
        ) AS latest_rows
        ORDER BY id DESC
    ');

    $rows = [];

    while ($row = $result->fetch_assoc())
    {
        $rows[] = $row;
    }

    echo json_encode(
        [
            'success' => true,
            'rows' => $rows
        ],
        JSON_UNESCAPED_UNICODE | JSON_UNESCAPED_SLASHES
    );
}
catch (Throwable $e)
{
    http_response_code(500);
    echo json_encode(
        [
            'success' => false,
            'error' => $e->getMessage()
        ],
        JSON_UNESCAPED_UNICODE | JSON_UNESCAPED_SLASHES
    );
}
