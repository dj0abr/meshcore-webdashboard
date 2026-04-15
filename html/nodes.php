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

    $sql = "
        SELECT
            n.id,
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
            COUNT(cm.id) AS msg_count
        FROM nodes n
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
            n.adv_lon_e6
        ORDER BY
            n.name ASC
    ";

    $result = $db->query($sql);

    $nodes = [];

    while ($row = $result->fetch_assoc())
    {
        $nodes[] =
        [
            'id' => (int) $row['id'],
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
            'adv_lat' => isset($row['adv_lat_e6']) ? ((int) $row['adv_lat_e6'] / 1000000.0) : null,
            'adv_lon' => isset($row['adv_lon_e6']) ? ((int) $row['adv_lon_e6'] / 1000000.0) : null,
            'msg_count' => (int) $row['msg_count'],
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