(function(global)
{
    "use strict";

    const Shared = global.MeshCoreShared || {};
    const fetchJson = Shared.fetchJson;

    if (typeof fetchJson !== "function")
    {
        throw new Error("MeshCoreShared.fetchJson is required before meshcore-api.js");
    }

    function jsonHeaders(extraHeaders = {})
    {
        return Object.assign(
        {
            "Accept": "application/json"
        }, extraHeaders);
    }

    function noStoreGet()
    {
        return {
            method: "GET",
            cache: "no-store",
            headers: jsonHeaders()
        };
    }

    function postJson(payload)
    {
        return {
            method: "POST",
            headers: jsonHeaders(
            {
                "Content-Type": "application/json"
            }),
            body: JSON.stringify(payload || {})
        };
    }

    function postEmpty()
    {
        return {
            method: "POST",
            headers: jsonHeaders()
        };
    }

    function withCacheBuster(url)
    {
        const separator = url.indexOf("?") === -1 ? "?" : "&";
        return `${url}${separator}_=${Date.now()}`;
    }

    async function loadNodes(type = "all")
    {
        return await fetchJson(
            withCacheBuster(`nodes.php?type=${encodeURIComponent(type)}`),
            noStoreGet()
        );
    }

    async function loadCompanionRadioStatus()
    {
        return await fetchJson("companion_radio_status.php", noStoreGet());
    }

    async function startDiscover()
    {
        return await fetchJson("discover_start.php", postEmpty());
    }

    async function loadDiscoverStatus()
    {
        return await fetchJson("discover_status.php", noStoreGet());
    }

    async function clearDiscoverRequest()
    {
        return await fetchJson("discover_clear.php", postEmpty());
    }

    async function startProtectedRepeaterRequest(targetName, password)
    {
        return await fetchJson("protected_repeater_request.php", postJson(
        {
            command: "start",
            target_name: targetName || "",
            auth_password: password || ""
        }));
    }

    async function loadProtectedRepeaterStatus()
    {
        return await fetchJson(
            withCacheBuster("protected_repeater_request.php?command=status"),
            noStoreGet()
        );
    }

    async function loadNodePath(nodeId)
    {
        return await fetchJson(
            withCacheBuster(`node_path.php?node_id=${encodeURIComponent(nodeId)}`),
            noStoreGet()
        );
    }

    async function loadMessagePath(correlationKey)
    {
        return await fetchJson(
            withCacheBuster(`message_path.php?correlation_key=${encodeURIComponent(correlationKey)}`),
            noStoreGet()
        );
    }

    async function loadMessages(params)
    {
        const chatKind = params && params.kind ? String(params.kind) : "direct";

        if (chatKind === "channel")
        {
            return await fetchJson(
                withCacheBuster(`messages.php?kind=channel&channel_key_hex=${encodeURIComponent(String(params.channel_key_hex || ""))}`),
                noStoreGet()
            );
        }

        return await fetchJson(
            withCacheBuster(`messages.php?kind=${encodeURIComponent(chatKind)}&name=${encodeURIComponent(String(params.name || ""))}`),
            noStoreGet()
        );
    }

    async function loadNewMessages(afterId)
    {
        return await fetchJson(
            `new_messages.php?after_id=${encodeURIComponent(afterId)}`,
            noStoreGet()
        );
    }

    async function saveRoomPassword(context, password)
    {
        return await fetchJson("save_room_password.php", postJson(
        {
            room_node_id: context.roomNodeId,
            room_name: context.roomName,
            password: password
        }));
    }

    async function loadTxStatus(txId)
    {
        return await fetchJson(
            `tx_status.php?id=${encodeURIComponent(txId)}`,
            {
                method: "GET",
                headers: jsonHeaders()
            }
        );
    }

    async function sendMessage(payload)
    {
        return await fetchJson("send_message.php", postJson(payload));
    }

    async function sendFloodAdvert()
    {
        return await sendMessage(
        {
            tx_kind: 2,
            message_text: "[flood advert]",
            max_retries: 1
        });
    }

    async function loadChannels()
    {
        return await fetchJson("channels.php", noStoreGet());
    }

    async function loadMonitor()
    {
        return await fetchJson(withCacheBuster("monitor.php"), noStoreGet());
    }

    async function saveChannel(payload)
    {
        return await fetchJson("save_channel.php", postJson(payload));
    }

    async function deleteChannel(keyHex)
    {
        return await fetchJson("delete_channel.php", postJson(
        {
            key_hex: String(keyHex || "")
        }));
    }

    async function loadCompanionSetup()
    {
        return await fetchJson("companion_setup_read.php", noStoreGet());
    }

    async function applyCompanionSetup(payload)
    {
        return await fetchJson("companion_setup.php", postJson(payload));
    }

    async function resetNodePath(publicKeyHex)
    {
        return await fetchJson("reset_node_path.php", postJson(
        {
            public_key_hex: String(publicKeyHex || "")
        }));
    }

    global.MeshCoreApi =
    {
        loadNodes,
        loadCompanionRadioStatus,
        startDiscover,
        loadDiscoverStatus,
        clearDiscoverRequest,
        startProtectedRepeaterRequest,
        loadProtectedRepeaterStatus,
        loadNodePath,
        loadMessagePath,
        loadMessages,
        loadNewMessages,
        saveRoomPassword,
        loadTxStatus,
        sendMessage,
        sendFloodAdvert,
        loadChannels,
        loadMonitor,
        saveChannel,
        deleteChannel,
        loadCompanionSetup,
        applyCompanionSetup,
        resetNodePath
    };
})(window);
