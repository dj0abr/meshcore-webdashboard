<?php
declare(strict_types=1);

header('Content-Type: application/json; charset=utf-8');

mysqli_report(MYSQLI_REPORT_ERROR | MYSQLI_REPORT_STRICT);

function advertTypeLabel(int $advertType): string
{
    switch ($advertType)
    {
        case 1:
            return 'CHAT';

        case 2:
            return 'REPEATER';

        case 3:
            return 'ROOM';

        case 4:
            return 'SENSOR';

        case 0:
            return 'UNKNOWN';

        default:
            return 'UNKNOWN';
    }
}

try
{
    $typeFilter = $_GET['type'] ?? 'all';
    $allowedTypes = ['all', 'chat', 'repeater', 'room', 'sensor'];

    if (!in_array($typeFilter, $allowedTypes, true))
    {
        $typeFilter = 'all';
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

    $nodeUnionSql = "
        SELECT
            CONCAT('nodes:', id) AS id,
            id AS db_id,
            'nodes' AS node_table,
            node_id,
            advert_type,
            name,
            public_key_hex,
            prefix6_hex,
            last_advert_at,
            first_seen_at,
            updated_at,
            last_mod_at,
            advert_flags,
            adv_lat_e6,
            adv_lon_e6
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
        SELECT
            CONCAT('repeaternodes:', id) AS id,
            id AS db_id,
            'repeaternodes' AS node_table,
            node_id,
            advert_type,
            name,
            public_key_hex,
            prefix6_hex,
            last_advert_at,
            first_seen_at,
            updated_at,
            last_mod_at,
            advert_flags,
            adv_lat_e6,
            adv_lon_e6
        FROM repeaternodes
        WHERE public_key_hex IS NOT NULL
    ";

    $sql = "
        SELECT
            n.id,
            n.db_id,
            n.node_table,
            n.node_id,
            n.advert_type,
            n.name,
            n.public_key_hex,
            n.prefix6_hex,
            n.last_advert_at,
            n.first_seen_at,
            n.updated_at,
            n.last_mod_at,
            n.advert_flags,
            n.adv_lat_e6,
            n.adv_lon_e6,
            p.path_len AS last_advert_path_len,
            p.path_hash_size AS last_advert_path_hash_size,
            p.path_text AS last_advert_path_text,
            p.last_seen_at AS last_advert_path_at,
            COUNT(cm.id) AS msg_count,
            MAX(cm.timestamp_epoch) AS newest_msg_epoch
        FROM (" . $nodeUnionSql . ") n
        LEFT JOIN node_advert_paths p
            ON LOWER(p.public_key_hex) = LOWER(n.public_key_hex)
        LEFT JOIN chat_messages cm
            ON cm.name = n.name
        AND (
                (n.advert_type = 1 AND cm.chat_kind = 0)
                OR
                (n.advert_type = 3 AND cm.chat_kind = 1)
        )
    ";

    if ($typeFilter === 'chat')
    {
        $sql .= " WHERE n.advert_type = 1";
    }
    else if ($typeFilter === 'repeater')
    {
        $sql .= " WHERE n.advert_type = 2";
    }
    else if ($typeFilter === 'room')
    {
        $sql .= " WHERE n.advert_type = 3";
    }
    else if ($typeFilter === 'sensor')
    {
        $sql .= " WHERE n.advert_type = 4";
    }

    $sql .= "
        GROUP BY
            n.id,
            n.db_id,
            n.node_table,
            n.node_id,
            n.advert_type,
            n.name,
            n.public_key_hex,
            n.prefix6_hex,
            n.last_advert_at,
            n.first_seen_at,
            n.updated_at,
            n.last_mod_at,
            n.advert_flags,
            n.adv_lat_e6,
            n.adv_lon_e6,
            p.path_len,
            p.path_hash_size,
            p.path_text,
            p.last_seen_at
        ORDER BY
            n.name ASC
    ";

    $result = $db->query($sql);

    $nodes = [];

    while ($row = $result->fetch_assoc())
    {
        $nodes[] =
        [
            'id' => $row['id'],
            'db_id' => (int) $row['db_id'],
            'node_table' => $row['node_table'],
            'node_id' => ($row['node_id'] !== null) ? (int) $row['node_id'] : null,
            'advert_type' => (int) $row['advert_type'],
            'advert_type_label' => advertTypeLabel((int) $row['advert_type']),
            'name' => $row['name'],
            'public_key_hex' => $row['public_key_hex'],
            'prefix6_hex' => $row['prefix6_hex'],
            'last_advert_at' => $row['last_advert_at'],
            'first_seen_at' => $row['first_seen_at'],
            'updated_at' => $row['updated_at'],
            'last_mod_at' => $row['last_mod_at'],
            'advert_flags' => ($row['advert_flags'] !== null) ? (int) $row['advert_flags'] : null,
            'adv_lat' => ($row['adv_lat_e6'] !== null) ? ((int) $row['adv_lat_e6'] / 1000000.0) : null,
            'adv_lon' => ($row['adv_lon_e6'] !== null) ? ((int) $row['adv_lon_e6'] / 1000000.0) : null,
            'last_advert_path_len' => ($row['last_advert_path_len'] !== null) ? (int) $row['last_advert_path_len'] : null,
            'last_advert_path_hash_size' => ($row['last_advert_path_hash_size'] !== null) ? (int) $row['last_advert_path_hash_size'] : null,
            'last_advert_path_text' => $row['last_advert_path_text'],
            'last_advert_path_at' => $row['last_advert_path_at'],
            'msg_count' => (int) $row['msg_count'],
            'newest_msg_epoch' => ($row['newest_msg_epoch'] !== null) ? (int) $row['newest_msg_epoch'] : null,
        ];
    }

    $result->free();
    $db->close();

    echo json_encode(
        [
            'success' => true,
            'nodes' => $nodes,
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