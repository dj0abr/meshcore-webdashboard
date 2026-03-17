let table = null;

const el =
{
    nodeCount: document.getElementById("nodeCount"),
    typeFilter: document.getElementById("typeFilter"),
    tableError: document.getElementById("tableError"),
    mapEmpty: document.getElementById("mapEmpty"),
    chatView: document.getElementById("chatView"),
    chatTitle: document.getElementById("chatTitle"),
    callsignFilter: document.getElementById("callsignFilter"),
    chatBody: document.getElementById("chatBody"),
    allMapButton: document.getElementById("allMapButton"),
    mapView: document.getElementById("mapView"),
    chatInput: document.getElementById("chatInput"),
    chatSendButton: document.getElementById("chatSendButton"),
    roomPasswordModal: document.getElementById("roomPasswordModal"),
    roomPasswordTitle: document.getElementById("roomPasswordTitle"),
    roomPasswordSubtitle: document.getElementById("roomPasswordSubtitle"),
    roomPasswordInput: document.getElementById("roomPasswordInput"),
    roomPasswordSaveButton: document.getElementById("roomPasswordSaveButton"),
    roomPasswordCancelButton: document.getElementById("roomPasswordCancelButton"),
    roomPasswordError: document.getElementById("roomPasswordError"),
    advertButton: document.getElementById("advertButton"),
    tabNodesBtn: document.getElementById("tabNodesBtn"),
    tabChannelsBtn: document.getElementById("tabChannelsBtn"),
    channelsList: document.getElementById("channelsList"),
    createPrivateChannelBtn: document.getElementById("createPrivateChannelBtn"),
    joinPrivateChannelBtn: document.getElementById("joinPrivateChannelBtn"),
    joinPublicChannelBtn: document.getElementById("joinPublicChannelBtn"),
    joinHashtagChannelBtn: document.getElementById("joinHashtagChannelBtn"),
    removeChannelBtn: document.getElementById("removeChannelBtn"),
    channelModal: document.getElementById("channelModal"),
    channelModalTitle: document.getElementById("channelModalTitle"),
    channelModalSubtitle: document.getElementById("channelModalSubtitle"),
    channelNameGroup: document.getElementById("channelNameGroup"),
    channelNameInput: document.getElementById("channelNameInput"),
    channelSecretGroup: document.getElementById("channelSecretGroup"),
    channelSecretInput: document.getElementById("channelSecretInput"),
    channelResultGroup: document.getElementById("channelResultGroup"),
    channelResultSecret: document.getElementById("channelResultSecret"),
    channelModalError: document.getElementById("channelModalError"),
    channelModalConfirmButton: document.getElementById("channelModalConfirmButton"),
    channelModalCancelButton: document.getElementById("channelModalCancelButton"),
    channelQrGroup: document.getElementById("channelQrGroup"),
    channelQrCode: document.getElementById("channelQrCode"),
    discoverButton: document.getElementById("discoverButton"),
    discoverModal: document.getElementById("discoverModal"),
    discoverStatusText: document.getElementById("discoverStatusText"),
    discoverJobInfo: document.getElementById("discoverJobInfo"),
    discoverResults: document.getElementById("discoverResults"),
    discoverRepeatButton: document.getElementById("discoverRepeatButton"),
    discoverCloseButton: document.getElementById("discoverCloseButton"),
    discoverModalError: document.getElementById("discoverModalError"),
    setupButton: document.getElementById("settingsButton"),
    setupModal: document.getElementById("setupModal"),
    setupNameInput: document.getElementById("setupNameInput"),
    setupLatInput: document.getElementById("setupLatInput"),
    setupLonInput: document.getElementById("setupLonInput"),
    setupApplyButton: document.getElementById("setupApplyButton"),
    setupCancelButton: document.getElementById("setupCancelButton"),
    setupModalError: document.getElementById("setupModalError"),
    pageTitle: document.getElementById("pageTitle"),
};

const state =
{
    leafletMap: null,
    leafletMarkers: null,
    autoZoom: true,
    rightView: "empty",
    chatRow: null,
    chatRefreshTimer: null,
    chatLastMessageId: 0,
    chatMessages: [],
    txPollTimers: new Map(),
    roomPasswordPrompt: null,
    openRoomPasswordNodeIds: new Set(),
    roomPasswordSuppressUntil: new Map(),
    channels: [],
    leftTab: "nodes",
    channelDialog: null,
    discoverPollTimer: null,
    discoverModalOpen: false,
    discoverPending: false,
    discoverPendingJobId: null,
};

const icons =
{
    chat: L.icon(
    {
        iconUrl: "marker-icon-green.png",
        iconSize: [25, 41],
        iconAnchor: [12, 41],
        popupAnchor: [1, -34],
        shadowUrl: "https://unpkg.com/leaflet@1.9.4/dist/images/marker-shadow.png",
        shadowSize: [41, 41]
    }),
    repeater: L.icon(
    {
        iconUrl: "marker-icon-blue.png",
        iconSize: [25, 41],
        iconAnchor: [12, 41],
        popupAnchor: [1, -34],
        shadowUrl: "https://unpkg.com/leaflet@1.9.4/dist/images/marker-shadow.png",
        shadowSize: [41, 41]
    }),
    room: L.icon(
    {
        iconUrl: "marker-icon-violet.png",
        iconSize: [25, 41],
        iconAnchor: [12, 41],
        popupAnchor: [1, -34],
        shadowUrl: "https://unpkg.com/leaflet@1.9.4/dist/images/marker-shadow.png",
        shadowSize: [41, 41]
    })
};

function getLocale()
{
    return getLanguage() === "en" ? "en-GB" : "de-DE";
}

function tr(key, fallback, vars = {})
{
    if (typeof t === "function")
    {
        return t(key, vars);
    }

    let text = fallback || key;

    Object.keys(vars).forEach(function(name)
    {
        text = text.replaceAll("{" + name + "}", String(vars[name]));
    });

    return text;
}

function getSelectedChannel()
{
    if (!state.chatRow || !isChannelRow(state.chatRow))
    {
        return null;
    }

    return state.chatRow;
}

function closeChannelDialog()
{
    state.channelDialog = null;

    if (!el.channelModal)
    {
        return;
    }

    el.channelModal.classList.remove("visible");
    el.channelModal.setAttribute("aria-hidden", "true");

    if (el.channelNameInput)
    {
        el.channelNameInput.value = "";
    }

    if (el.channelSecretInput)
    {
        el.channelSecretInput.value = "";
    }

    if (el.channelResultSecret)
    {
        el.channelResultSecret.value = "";
    }

    if (el.channelQrCode)
    {
        el.channelQrCode.innerHTML = "";
    }

    if (el.channelQrGroup)
    {
        el.channelQrGroup.style.display = "none";
    }

    if (el.channelModalError)
    {
        el.channelModalError.textContent = "";
        el.channelModalError.style.display = "none";
    }

    if (el.channelResultGroup)
    {
        el.channelResultGroup.style.display = "none";
    }

    if (el.channelNameGroup)
    {
        el.channelNameGroup.style.display = "";
    }

    if (el.channelSecretGroup)
    {
        el.channelSecretGroup.style.display = "";
    }

    if (el.channelModalConfirmButton)
    {
        el.channelModalConfirmButton.disabled = false;
        el.channelModalConfirmButton.textContent = tr("channel.ok", "OK");
    }

    if (el.channelModalCancelButton)
    {
        el.channelModalCancelButton.disabled = false;
        el.channelModalCancelButton.style.display = "";
    }
}

function buildMeshCoreChannelQrPayload(channelName, secret)
{
    return `meshcore://channel/add?name=${encodeURIComponent(channelName)}&secret=${encodeURIComponent(secret)}`;
}

function renderChannelQrCode(payload)
{
    if (!el.channelQrCode || !el.channelQrGroup)
    {
        return;
    }

    el.channelQrCode.innerHTML = "";
    el.channelQrGroup.style.display = "";

    if (typeof QRCode === "undefined")
    {
        el.channelQrCode.textContent = tr("error.qr_library_missing", "QR-Code Bibliothek nicht geladen.");
        return;
    }

    new QRCode(el.channelQrCode,
    {
        text: payload,
        width: 220,
        height: 220,
        correctLevel: QRCode.CorrectLevel.M
    });
}

function openChannelDialog(action)
{
    state.channelDialog =
    {
        action: action
    };

    if (!el.channelModal)
    {
        return;
    }

    if (el.channelModalError)
    {
        el.channelModalError.textContent = "";
        el.channelModalError.style.display = "none";
    }

    if (el.channelResultGroup)
    {
        el.channelResultGroup.style.display = "none";
    }

    if (el.channelNameInput)
    {
        el.channelNameInput.value = "";
    }

    if (el.channelSecretInput)
    {
        el.channelSecretInput.value = "";
    }

    if (el.channelNameGroup)
    {
        el.channelNameGroup.style.display = "";
    }

    if (el.channelSecretGroup)
    {
        el.channelSecretGroup.style.display = "none";
    }

    if (el.channelModalConfirmButton)
    {
        el.channelModalConfirmButton.disabled = false;
    }

    if (el.channelModalCancelButton)
    {
        el.channelModalCancelButton.disabled = false;
    }

    switch (action)
    {
        case "create_private":
            el.channelModalTitle.textContent = tr("channel.action.create_private.title", "Create private channel");
            el.channelModalSubtitle.textContent = tr(
                "channel.action.create_private.subtitle",
                "Neuen privaten Channel anlegen. Der Secret Key wird anschließend angezeigt."
            );
            el.channelModalConfirmButton.textContent = tr("channel.action.create_private.confirm", "Create");
            el.channelSecretGroup.style.display = "none";
            break;

        case "join_private":
            el.channelModalTitle.textContent = tr("channel.action.join_private.title", "Join private channel");
            el.channelModalSubtitle.textContent = tr(
                "channel.action.join_private.subtitle",
                "Channelname und Secret Key eingeben."
            );
            el.channelModalConfirmButton.textContent = tr("channel.action.join_private.confirm", "Join");
            el.channelSecretGroup.style.display = "";
            break;

        case "join_public":
            el.channelModalTitle.textContent = tr("channel.action.join_public.title", "Join public channel");
            el.channelModalSubtitle.textContent = tr(
                "channel.action.join_public.subtitle",
                "Öffentlichen Channel über den Namen beitreten."
            );
            el.channelModalConfirmButton.textContent = tr("channel.action.join_public.confirm", "Join");
            el.channelSecretGroup.style.display = "none";
            break;

        case "join_hashtag":
            el.channelModalTitle.textContent = tr("channel.action.join_hashtag.title", "Join hashtag channel");
            el.channelModalSubtitle.textContent = tr(
                "channel.action.join_hashtag.subtitle",
                "Hashtag-Channel eingeben, z. B. #drones."
            );
            el.channelModalConfirmButton.textContent = tr("channel.action.join_hashtag.confirm", "Join");
            el.channelSecretGroup.style.display = "none";
            break;

        case "remove":
        {
            const selected = getSelectedChannel();

            el.channelModalTitle.textContent = tr("channel.action.remove.title", "Remove channel");
            el.channelModalSubtitle.textContent = selected
                ? tr(
                    "channel.action.remove.subtitle",
                    "Channel wirklich entfernen: {name} (IDX {idx})",
                    {
                        name: selected.name,
                        idx: selected.channel_idx
                    }
                )
                : tr("channel.action.remove.none_selected", "Kein Channel ausgewählt.");
            el.channelModalConfirmButton.textContent = tr("channel.action.remove.confirm", "Remove");
            el.channelNameGroup.style.display = "none";
            el.channelSecretGroup.style.display = "none";
            break;
        }

        default:
            break;
    }

    el.channelModal.classList.add("visible");
    el.channelModal.setAttribute("aria-hidden", "false");

    setTimeout(function()
    {
        if (action !== "remove" && el.channelNameInput)
        {
            el.channelNameInput.focus();
        }
    }, 0);
}

function showChannelDialogError(message)
{
    if (!el.channelModalError)
    {
        return;
    }

    el.channelModalError.textContent = message || tr("error.unknown", "Unbekannter Fehler");
    el.channelModalError.style.display = "block";
}

function escapeHtml(value)
{
    return String(value)
        .replaceAll("&", "&amp;")
        .replaceAll("<", "&lt;")
        .replaceAll(">", "&gt;")
        .replaceAll('"', "&quot;")
        .replaceAll("'", "&#39;");
}

function isChatNode(row)
{
    return row && row.advert_type_label === "CHAT";
}

function isRoomNode(row)
{
    return row && row.advert_type_label === "ROOM";
}

function isChannelRow(row)
{
    return row && row.type === "channel";
}

function isChatLikeNode(row)
{
    return isChatNode(row) || isRoomNode(row) || isChannelRow(row);
}

function getChatKindLabel(row)
{
    if (isRoomNode(row))
    {
        return tr("channel.type.room", "Room");
    }

    if (isChannelRow(row))
    {
        return tr("channel.type.channel", "Channel");
    }

    return tr("channel.type.chat", "Chat");
}

function getChatKindValue(row)
{
    if (isRoomNode(row))
    {
        return tr("channel.kind.room", "room");
    }

    if (isChannelRow(row))
    {
        return tr("channel.kind.channel", "channel");
    }

    return tr("channel.kind.dm", "dm");
}

function getMarkerIcon(row)
{
    switch (row.advert_type_label)
    {
        case "CHAT":
            return icons.chat;

        case "REPEATER":
            return icons.repeater;

        case "ROOM":
            return icons.room;

        case "SENSOR":
            return icons.chat;

        default:
            return icons.chat;
    }
}

function parseMariaDbDateTime(value)
{
    if (!value || value === "0000-00-00 00:00:00")
    {
        return 0;
    }

    return new Date(value.replace(" ", "T")).getTime();
}

function formatDateTime(value)
{
    if (!value)
    {
        return "?";
    }

    const date = new Date(value.replace(" ", "T"));

    if (Number.isNaN(date.getTime()))
    {
        return value;
    }

    return date.toLocaleString(getLocale());
}

function formatEpochDateTime(epochSeconds)
{
    const value = Number(epochSeconds || 0);

    if (!Number.isFinite(value) || value <= 0)
    {
        return "-";
    }

    return new Date(value * 1000).toLocaleString(getLocale());
}

function containsPossibleCallsign(value)
{
    if (!value)
    {
        return false;
    }

    const text = String(value).toUpperCase();
    const regex = /(^|[^A-Z0-9])([A-Z]{1,2}[0-9][A-Z]{1,3})(?=$|[^A-Z0-9])/;

    return regex.test(text);
}

function getNodeLatLon(row)
{
    let lat = null;
    let lon = null;

    if (row.adv_lat !== null && row.adv_lat !== undefined)
    {
        lat = Number(row.adv_lat);
    }
    else if (row.adv_lat_e6 !== null && row.adv_lat_e6 !== undefined)
    {
        lat = Number(row.adv_lat_e6) / 1000000.0;
    }

    if (row.adv_lon !== null && row.adv_lon !== undefined)
    {
        lon = Number(row.adv_lon);
    }
    else if (row.adv_lon_e6 !== null && row.adv_lon_e6 !== undefined)
    {
        lon = Number(row.adv_lon_e6) / 1000000.0;
    }

    const valid =
        Number.isFinite(lat) &&
        Number.isFinite(lon) &&
        !(lat === 0 && lon === 0);

    if (!valid)
    {
        return null;
    }

    return { lat: lat, lon: lon };
}

function hasLocation(row)
{
    return getNodeLatLon(row) !== null;
}

function isRoomPasswordPromptSuppressed(roomNodeId)
{
    const key = String(roomNodeId || "");
    const until = Number(state.roomPasswordSuppressUntil.get(key) || 0);

    if (until <= 0)
    {
        return false;
    }

    if (Date.now() >= until)
    {
        state.roomPasswordSuppressUntil.delete(key);
        return false;
    }

    return true;
}

function setChatInputEnabled(enabled)
{
    if (el.chatInput)
    {
        el.chatInput.disabled = !enabled;
    }

    if (el.chatSendButton)
    {
        el.chatSendButton.disabled = !enabled;
    }
}

function scrollChatToBottom()
{
    if (!el.chatBody)
    {
        return;
    }

    requestAnimationFrame(function()
    {
        el.chatBody.scrollTop = el.chatBody.scrollHeight;
    });
}

function isChatNearBottom()
{
    if (!el.chatBody)
    {
        return true;
    }

    const threshold = 80;

    return (el.chatBody.scrollHeight - el.chatBody.scrollTop - el.chatBody.clientHeight) < threshold;
}

function resetChatState()
{
    state.chatRow = null;
    state.chatLastMessageId = 0;
    state.chatMessages = [];
}

function stopCurrentChatRefresh()
{
    if (state.chatRefreshTimer)
    {
        clearInterval(state.chatRefreshTimer);
        state.chatRefreshTimer = null;
    }
}

function stopTxPolling(txId)
{
    const timer = state.txPollTimers.get(txId);

    if (timer)
    {
        clearInterval(timer);
        state.txPollTimers.delete(txId);
    }
}

function hideRightPanelViews()
{
    if (el.mapView)
    {
        el.mapView.style.display = "none";
    }

    if (el.chatView)
    {
        el.chatView.style.display = "none";
    }

    if (el.mapEmpty)
    {
        el.mapEmpty.style.display = "none";
    }
}

function ensureMap()
{
    if (!el.mapView)
    {
        return null;
    }

    if (!state.leafletMap)
    {
        state.leafletMap = L.map("mapView",
        {
            zoomControl: true
        });

        L.tileLayer("https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png",
        {
            maxZoom: 19,
            attribution: "&copy; OpenStreetMap"
        }).addTo(state.leafletMap);

        state.leafletMarkers = L.layerGroup().addTo(state.leafletMap);
    }

    return state.leafletMap;
}

async function fetchJson(url, options = {})
{
    const response = await fetch(url, options);

    let data = null;

    try
    {
        data = await response.json();
    }
    catch (jsonError)
    {
        throw new Error(`HTTP ${response.status} (${tr("error.invalid_json", "keine gültige JSON-Antwort")})`);
    }

    if (!response.ok)
    {
        throw new Error(data?.error || `HTTP ${response.status}`);
    }

    if (!data || data.success === false)
    {
        throw new Error(data?.error || tr("error.unknown", "Unbekannter Fehler"));
    }

    return data;
}

async function startDiscover()
{
    return await fetchJson("discover_start.php",
    {
        method: "POST",
        headers:
        {
            "Accept": "application/json"
        }
    });
}

async function loadDiscoverStatus()
{
    return await fetchJson("discover_status.php",
    {
        method: "GET",
        cache: "no-store",
        headers:
        {
            "Accept": "application/json"
        }
    });
}

async function clearDiscoverRequest()
{
    return await fetchJson("discover_clear.php",
    {
        method: "POST",
        headers:
        {
            "Accept": "application/json"
        }
    });
}

function renderDiscoverResults(results)
{
    if (!el.discoverResults)
    {
        return;
    }

    el.discoverResults.innerHTML = "";

    if (!Array.isArray(results) || results.length === 0)
    {
        el.discoverResults.innerHTML =
            '<div class="discover-empty">' +
            escapeHtml(tr("discover.results.none", "Noch keine Ergebnisse.")) +
            '</div>';
        return;
    }

    results.forEach(function(row)
    {
        const item = document.createElement("div");
        item.className = "discover-result-card";

        item.innerHTML =
            `<div class="discover-result-title">${escapeHtml(row.node_name || row.node_id_hex)}</div>` +
            `<div class="discover-result-meta">` +
                `<span>ID: ${escapeHtml(row.node_id_hex)}</span>` +
                `<span>SNR RX: ${row.snr_rx_db}</span>` +
                `<span>SNR TX: ${row.snr_tx_db}</span>` +
                `<span>RSSI: ${row.rssi_dbm}</span>` +
                `<span>${escapeHtml(tr("discover.updated", "Update"))}: ${escapeHtml(row.updated_at || "-")}</span>` +
            `</div>`;

        el.discoverResults.appendChild(item);
    });
}

function discoverStatusSymbol(status)
{
    switch (status)
    {
        case "queued":
            return { icon: "⏳", title: tr("discover.status.queued", "Warteschlange") };

        case "running":
            return { icon: "⚙", title: tr("discover.status.running", "läuft") };

        case "done":
            return { icon: "✔", title: tr("discover.status.done", "fertig") };

        case "failed":
            return { icon: "✖", title: tr("discover.status.failed", "Fehler") };

        case "skipped":
            return { icon: "⤼", title: tr("discover.status.skipped", "übersprungen") };

        default:
            return { icon: "?", title: status || tr("discover.status.unknown_fallback", "unbekannt") };
    }
}

function setDiscoverStatusDisplay(status)
{
    if (!el.discoverStatusText)
    {
        return;
    }

    if (!status)
    {
        el.discoverStatusText.textContent = "-";
        el.discoverStatusText.title = "";
        return;
    }

    const symbol = discoverStatusSymbol(status);
    el.discoverStatusText.title = symbol.title;

    if (status === "running")
    {
        el.discoverStatusText.innerHTML = '<span class="status-spinner"></span>';
        return;
    }

    el.discoverStatusText.textContent = symbol.icon;
}

function setDiscoverStartButtonVisible(visible)
{
    if (!el.discoverRepeatButton)
    {
        return;
    }

    el.discoverRepeatButton.style.display = visible ? "" : "none";
    el.discoverRepeatButton.disabled = !visible;
}

function resetDiscoverPendingState()
{
    state.discoverPending = false;
    state.discoverPendingJobId = null;
}

function formatDiscoverJobInfo(job)
{
    if (!job)
    {
        return "-";
    }

    const symbol = discoverStatusSymbol(job.status_text);
    const parts =
    [
        `Job ${job.id}`,
        `${symbol.icon} ${symbol.title}`,
        `${tr("discover.results_count", "Treffer")}: ${job.result_count}`,
    ];

    if (job.finished_at)
    {
        parts.push(`${tr("discover.finished", "beendet")}: ${job.finished_at}`);
    }
    else if (job.started_at)
    {
        parts.push(`${tr("discover.started", "gestartet")}: ${job.started_at}`);
    }
    else if (job.created_at)
    {
        parts.push(`${tr("discover.created", "angelegt")}: ${job.created_at}`);
    }

    if (job.error_text)
    {
        parts.push(`${tr("discover.error_label", "Fehler")}: ${job.error_text}`);
    }

    return parts.join(", ");
}

function renderDiscoverIdleState()
{
    setDiscoverStatusDisplay(null);

    if (el.discoverJobInfo)
    {
        el.discoverJobInfo.textContent = "-";
    }

    renderDiscoverResults([]);
    setDiscoverStartButtonVisible(true);
}

function renderDiscoverQueuedState()
{
    setDiscoverStatusDisplay("queued");

    if (el.discoverJobInfo)
    {
        el.discoverJobInfo.textContent = "-";
    }

    renderDiscoverResults([]);
    setDiscoverStartButtonVisible(false);
}

function renderDiscoverJobState(job, results)
{
    setDiscoverStatusDisplay(job ? job.status_text : null);

    if (el.discoverJobInfo)
    {
        el.discoverJobInfo.textContent = formatDiscoverJobInfo(job);
    }

    renderDiscoverResults(results);

    if (!job)
    {
        setDiscoverStartButtonVisible(true);
        return;
    }

    switch (job.status_text)
    {
        case "queued":
        case "running":
            setDiscoverStartButtonVisible(false);
            break;

        case "done":
        case "failed":
        case "skipped":
        default:
            setDiscoverStartButtonVisible(true);
            break;
    }
}

function renderDiscoverStatus(data)
{
    const job = data && data.job ? data.job : null;
    const results = data && Array.isArray(data.results) ? data.results : [];

    if (state.discoverPending)
    {
        if (!job)
        {
            renderDiscoverQueuedState();
            return;
        }

        if (
            state.discoverPendingJobId === null ||
            Number(job.id) < Number(state.discoverPendingJobId)
        )
        {
            renderDiscoverQueuedState();
            return;
        }

        resetDiscoverPendingState();
    }

    if (!job)
    {
        renderDiscoverIdleState();
        return;
    }

    renderDiscoverJobState(job, results);
}

async function handleDiscoverStartClick()
{
    if (state.discoverPending)
    {
        return;
    }

    state.discoverPending = true;
    state.discoverPendingJobId = null;

    if (el.discoverModalError)
    {
        el.discoverModalError.textContent = "";
        el.discoverModalError.style.display = "none";
    }

    renderDiscoverQueuedState();

    try
    {
        const response = await startDiscover();

        state.discoverPendingJobId = response && response.job_id
            ? Number(response.job_id)
            : null;

        await refreshDiscoverModal();
    }
    catch (error)
    {
        resetDiscoverPendingState();
        renderDiscoverIdleState();

        if (el.discoverModalError)
        {
            el.discoverModalError.textContent =
                error.message || tr("discover.error.start_failed", "Discover konnte nicht gestartet werden.");
            el.discoverModalError.style.display = "block";
        }
    }
}

async function refreshDiscoverModal()
{
    try
    {
        const data = await loadDiscoverStatus();

        if (el.discoverModalError)
        {
            el.discoverModalError.textContent = "";
            el.discoverModalError.style.display = "none";
        }

        renderDiscoverStatus(data);
    }
    catch (error)
    {
        if (el.discoverModalError)
        {
            el.discoverModalError.textContent =
                error.message || tr("discover.error.status_failed", "Discover-Status konnte nicht geladen werden.");
            el.discoverModalError.style.display = "block";
        }
    }
}

function stopDiscoverPolling()
{
    if (state.discoverPollTimer)
    {
        clearInterval(state.discoverPollTimer);
        state.discoverPollTimer = null;
    }
}

function startDiscoverPolling()
{
    stopDiscoverPolling();

    state.discoverPollTimer = setInterval(function()
    {
        if (!state.discoverModalOpen)
        {
            return;
        }

        refreshDiscoverModal();
    }, 1000);
}

async function openDiscoverDialog()
{
    if (!el.discoverModal)
    {
        return;
    }

    state.discoverModalOpen = true;

    if (el.discoverModalError)
    {
        el.discoverModalError.textContent = "";
        el.discoverModalError.style.display = "none";
    }

    el.discoverModal.classList.add("visible");
    el.discoverModal.setAttribute("aria-hidden", "false");

    await refreshDiscoverModal();
    startDiscoverPolling();
}

async function closeDiscoverDialog()
{
    state.discoverModalOpen = false;
    stopDiscoverPolling();

    try
    {
        await clearDiscoverRequest();
    }
    catch (error)
    {
        console.warn("discover_clear failed:", error);
    }

    if (!el.discoverModal)
    {
        return;
    }

    el.discoverModal.classList.remove("visible");
    el.discoverModal.setAttribute("aria-hidden", "true");

    if (el.discoverModalError)
    {
        el.discoverModalError.textContent = "";
        el.discoverModalError.style.display = "none";
    }
}

function showEmptyRightPanel()
{
    stopCurrentChatRefresh();
    resetChatState();
    state.rightView = "empty";

    setChatInputEnabled(false);
    hideRightPanelViews();

    if (el.mapEmpty)
    {
        el.mapEmpty.innerHTML = escapeHtml(
            tr(
                "map.empty.default",
                "Linksklick auf Chat, Room oder Channel = Verlauf, Rechtsklick = Karte."
            )
        );
        el.mapEmpty.style.display = "grid";
    }
}

function showInfoForRow(row)
{
    stopCurrentChatRefresh();
    resetChatState();

    setChatInputEnabled(false);
    hideRightPanelViews();

    if (el.mapEmpty)
    {
        el.mapEmpty.innerHTML =
            `📍 <strong>${escapeHtml(row.name || "-")}</strong> ` +
            escapeHtml(tr("map.no_position_for_node", "keine Positionsdaten verfügbar."));
        el.mapEmpty.style.display = "grid";
    }
}

function showMapForRow(row)
{
    state.rightView = "map";
    resetChatState();
    setChatInputEnabled(false);

    const pos = getNodeLatLon(row);

    if (!pos || !el.mapView)
    {
        return;
    }

    hideRightPanelViews();
    el.mapView.style.display = "block";

    const map = ensureMap();

    if (!map || !state.leafletMarkers)
    {
        return;
    }

    state.leafletMarkers.clearLayers();

    const marker = L.marker([pos.lat, pos.lon], { icon: getMarkerIcon(row) }).addTo(state.leafletMarkers);
    marker.bindPopup(escapeHtml(row.name || "-"));
    if (state.autoZoom)
    {
        map.setView([pos.lat, pos.lon], 13);
    }

    setTimeout(function()
    {
        map.invalidateSize();
    }, 0);
}

function showAllNodesMap()
{
    stopCurrentChatRefresh();
    resetChatState();
    state.rightView = "allmap";
    setChatInputEnabled(false);

    if (!el.mapView)
    {
        return;
    }

    const rows = table.getData();
    const nodesWithPos = rows.filter(function(row)
    {
        return hasLocation(row);
    });

    hideRightPanelViews();
    el.mapView.style.display = "block";

    const map = ensureMap();

    if (!map || !state.leafletMarkers)
    {
        return;
    }

    state.leafletMarkers.clearLayers();

    if (nodesWithPos.length === 0)
    {
        showEmptyRightPanel();

        if (el.mapEmpty)
        {
            el.mapEmpty.innerHTML = escapeHtml(
                tr("map.empty.no_nodes", "Keine Nodes mit Position vorhanden.")
            );
        }

        return;
    }

    const bounds = [];

    nodesWithPos.forEach(function(row)
    {
        const pos = getNodeLatLon(row);

        if (!pos)
        {
            return;
        }

        const marker = L.marker([pos.lat, pos.lon], { icon: getMarkerIcon(row) }).addTo(state.leafletMarkers);
        marker.bindPopup(`<strong>${escapeHtml(row.name || "-")}</strong><br>${escapeHtml(row.advert_type_label || "")}`);
        bounds.push([pos.lat, pos.lon]);
    });

    setTimeout(function()
    {
        map.invalidateSize();

        if (state.autoZoom)
        {
            if (bounds.length === 1)
            {
                map.setView(bounds[0], 13);
            }
            else
            {
                map.fitBounds(bounds, { padding: [30, 30] });
            }
            state.autoZoom = false;
        }
    }, 0);
}

function relativeTime(cell)
{
    const value = cell.getRow().getData().last_advert_at;

    if (!value)
    {
        return '<div class="last-seen unknown">?</div>';
    }

    const timestamp = parseMariaDbDateTime(value);

    if (!timestamp)
    {
        return '<div class="last-seen unknown">?</div>';
    }

    const diff = Math.floor((Date.now() - timestamp) / 1000);

    if (diff <= 0)
    {
        return '<div class="last-seen unknown">?</div>';
    }

    let text = "";
    let status = "red";

    if (diff < 60)
    {
        text = diff + "s";
    }
    else if (diff < 3600)
    {
        text = Math.floor(diff / 60) + "m";
    }
    else if (diff < 86400)
    {
        const hours = Math.floor(diff / 3600);
        const minutes = Math.floor((diff % 3600) / 60);

        const minStr = String(minutes).padStart(2, "0");

        text = hours + ":" + minStr + "h";
    }
    else
    {
        text = Math.floor(diff / 86400) + "d";
    }

    if (diff < 3600)
    {
        status = "green";
    }
    else if (diff < 5 * 3600)
    {
        status = "yellow";
    }

    return `
        <div class="last-seen">
            <span class="status-dot ${status}"></span>
            <span class="last-seen-text">${text}</span>
        </div>
    `;
}

function lastAdvertSorter(a, b)
{
    function getTs(value)
    {
        if (!value)
        {
            return null;
        }

        const ts = parseMariaDbDateTime(value);

        if (!ts || ts <= 0)
        {
            return null;
        }

        return ts;
    }

    const aTs = getTs(a);
    const bTs = getTs(b);

    if (aTs === null && bTs === null)
    {
        return 0;
    }

    if (aTs === null)
    {
        return 1;
    }

    if (bTs === null)
    {
        return -1;
    }

    return aTs - bTs;
}

function advertTypeFormatter(cell)
{
    const row = cell.getRow().getData();
    const type = row.advert_type_label;

    let icon = "";
    let text = "";

    switch (type)
    {
        case "CHAT":
            icon = "chat.svg";
            text = tr("type.chat", "Chat");
            break;

        case "REPEATER":
            icon = "repeater.svg";
            text = tr("type.repeater", "Repeater");
            break;

        case "ROOM":
            icon = "room.svg";
            text = tr("type.room", "Room");
            break;

        case "SENSOR":
            icon = "sensor.png";
            text = tr("type.sensor", "Sensor");
            break;

        case "UNKNOWN":
            text = tr("type.unknown", "Unknown");
            break;

        default:
            text = String(type || tr("type.unknown", "Unknown"));
            break;
    }

    if (icon !== "")
    {
        return `
            <div class="type-cell">
                <img class="type-icon" src="${icon}" alt="">
                <span class="type-text">${text}</span>
            </div>
        `;
    }

    return `
        <div class="type-cell">
            <span class="type-text">${escapeHtml(text)}</span>
        </div>
    `;
}

function nameFormatter(cell)
{
    const row = cell.getRow().getData();
    const name = row.name || "-";
    const safeName = escapeHtml(name);
    const msgCount = Number(row.msg_count) || 0;

    const badge = msgCount > 0
        ? `<span class="message-badge">💬 ${msgCount}</span>`
        : "";

    if (isChatLikeNode(row) || hasLocation(row))
    {
        return `
            <span class="node-name-wrap">
                <span class="node-link">${safeName}</span>
                ${badge}
            </span>
        `;
    }

    return `
        <span class="node-name-wrap">
            <span>${safeName}</span>
            ${badge}
        </span>
    `;
}

function getOutgoingStatusClass(msg)
{
    const statusValue = Number(msg.status || 0);
    const uiState = String(msg.ui_state || "");

    if (uiState === "failed" || statusValue === 2)
    {
        return "error";
    }

    if (uiState === "confirmed" || statusValue === 1)
    {
        return "ok";
    }

    return "";
}

function renderOutgoingMessage(msg)
{
    const dateText = escapeHtml(
        Number(msg.timestamp_epoch || 0) > 0
            ? formatEpochDateTime(msg.timestamp_epoch)
            : formatDateTime(msg.received_at)
    );

    const textValue = escapeHtml(msg.text || msg.message_text || "");
    const statusText = escapeHtml(String(msg.ui_status_text || tr("chat.status.pending", "Pending")));
    const statusClass = getOutgoingStatusClass(msg);

    return `
        <div class="chat-message outgoing" data-tx-id="${escapeHtml(String(msg.tx_outbox_id || ""))}">
            <div class="chat-message-header">
                <div class="chat-message-meta">
                    <span>${dateText}</span>
                    <span>${escapeHtml(tr("chat.you", "Du"))}</span>
                </div>
            </div>
            <div class="chat-message-text">${textValue}</div>
            <div class="chat-message-status ${statusClass}">${statusText}</div>
        </div>
    `;
}

function renderIncomingMessage(msg)
{
    const dateText = escapeHtml(
        Number(msg.timestamp_epoch || 0) > 0
            ? formatEpochDateTime(msg.timestamp_epoch)
            : formatDateTime(msg.received_at)
    );

    const textValue = escapeHtml(msg.text || msg.message_text || "");
    const snr = (msg.snr_db !== null && msg.snr_db !== undefined) ? `${msg.snr_db} dB` : "-";
    const path = (msg.path_len !== null && msg.path_len !== undefined) ? `${msg.path_len} Hop(s)` : "-";

    const isRoomMessage =
        Number(msg.chat_kind || 0) === 1 ||
        String(msg.kind || "").toLowerCase() === "room";

    const roomSenderName = String(msg.room_sender_name || "").trim();

    const senderBlock =
        isRoomMessage && roomSenderName !== ""
            ? `<div class="chat-room-sender"><strong>${escapeHtml(roomSenderName)}</strong></div>`
            : "";

    return `
        <div class="chat-message">
            <div class="chat-message-header">
                <div class="chat-message-meta">
                    <span>${dateText}</span>
                    <span>${escapeHtml(tr("chat.snr", "SNR"))}: ${escapeHtml(snr)}</span>
                    <span>${escapeHtml(tr("chat.path", "Path"))}: ${escapeHtml(path)}</span>
                </div>
            </div>
            ${senderBlock}
            <div class="chat-message-text">${textValue}</div>
        </div>
    `;
}

function renderChatMessages(messages)
{
    if (!el.chatBody)
    {
        return;
    }

    const shouldAutoScroll = isChatNearBottom();
    state.chatMessages = Array.isArray(messages) ? messages : [];

    if (state.chatMessages.length === 0)
    {
        el.chatBody.innerHTML =
            '<div class="chat-empty">' +
            escapeHtml(tr("chat.empty", "Keine Messages für diesen Eintrag gefunden.")) +
            '</div>';

        if (shouldAutoScroll)
        {
            scrollChatToBottom();
        }

        return;
    }

    const html = state.chatMessages.map(function(msg)
    {
        return Number(msg.direction || 0) === 1
            ? renderOutgoingMessage(msg)
            : renderIncomingMessage(msg);
    }).join("");

    el.chatBody.innerHTML = `<div class="chat-messages">${html}</div>`;

    if (shouldAutoScroll)
    {
        scrollChatToBottom();
    }
}

async function loadChatMessages(row, keepScrollIfPossible = true)
{
    if (!row || !el.chatBody)
    {
        return;
    }

    const chatName = row.name || getChatKindLabel(row);
    const chatKind = getChatKindValue(row);
    const wasNearBottom = isChatNearBottom();

    let url = "";
    if (chatKind === "channel")
    {
        url =
            `messages.php?kind=channel&channel_idx=${encodeURIComponent(Number(row.channel_idx || 0))}&_=${Date.now()}`;
    }
    else
    {
        url =
            `messages.php?kind=${encodeURIComponent(chatKind)}&name=${encodeURIComponent(chatName)}&_=${Date.now()}`;
    }

    const data = await fetchJson(
        url,
        {
            method: "GET",
            cache: "no-store",
            headers:
            {
                "Accept": "application/json"
            }
        }
    );

    const messages = data.messages || [];
    const newestId = messages.length > 0 ? Number(messages[messages.length - 1].id || 0) : 0;
    const hasNewMessages = newestId > state.chatLastMessageId;

    renderChatMessages(messages);
    state.chatLastMessageId = newestId;

    if (!keepScrollIfPossible || wasNearBottom || hasNewMessages)
    {
        scrollChatToBottom();
    }
}

function startCurrentChatRefresh()
{
    stopCurrentChatRefresh();

    state.chatRefreshTimer = setInterval(async function()
    {
        if (!state.chatRow || !el.chatView || state.rightView !== "chat")
        {
            stopCurrentChatRefresh();
            return;
        }

        try
        {
            await loadChatMessages(state.chatRow, true);
        }
        catch (error)
        {
            console.error("Chat-Refresh fehlgeschlagen:", error);
        }
    }, 2000);
}

async function showChatForRow(row)
{
    state.rightView = "chat";
    state.chatRow = row;
    state.chatLastMessageId = 0;
    state.chatMessages = [];
    setChatInputEnabled(true);

    if (!row || !el.chatView || !el.chatTitle || !el.chatBody)
    {
        return;
    }

    hideRightPanelViews();

    const chatName = row.name || getChatKindLabel(row);
    el.chatTitle.textContent = `${getChatKindLabel(row)}: ${chatName}`;
    el.chatBody.innerHTML =
        '<div class="chat-empty">' +
        escapeHtml(tr("chat.loading", "Lade Messages ...")) +
        '</div>';
    el.chatView.style.display = "flex";

    try
    {
        await loadChatMessages(row, false);

        if (el.chatInput)
        {
            el.chatInput.focus();
        }

        startCurrentChatRefresh(row);
    }
    catch (error)
    {
        el.chatBody.innerHTML = `
            <div class="chat-error">
                ${escapeHtml(tr("chat.error_loading", "Fehler beim Laden der Messages: {message}",
                {
                    message: error.message || tr("error.unknown", "Unbekannter Fehler")
                }))}
            </div>
        `;
    }
}

function closeRoomPasswordDialog()
{
    if (state.roomPasswordPrompt && state.roomPasswordPrompt.roomNodeId)
    {
        state.openRoomPasswordNodeIds.delete(String(state.roomPasswordPrompt.roomNodeId));
    }

    state.roomPasswordPrompt = null;

    if (!el.roomPasswordModal)
    {
        return;
    }

    el.roomPasswordModal.classList.remove("visible");
    el.roomPasswordModal.setAttribute("aria-hidden", "true");

    if (el.roomPasswordInput)
    {
        el.roomPasswordInput.value = "";
    }

    if (el.roomPasswordError)
    {
        el.roomPasswordError.textContent = "";
        el.roomPasswordError.style.display = "none";
    }
}

function openRoomPasswordDialog(context)
{
    if (!el.roomPasswordModal)
    {
        return;
    }

    const roomKey = String(context.roomNodeId || "");

    if (roomKey !== "" && state.openRoomPasswordNodeIds.has(roomKey))
    {
        return;
    }

    if (roomKey !== "")
    {
        state.openRoomPasswordNodeIds.add(roomKey);
    }

    state.roomPasswordPrompt = context;

    if (el.roomPasswordTitle)
    {
        el.roomPasswordTitle.textContent = tr(
            "room_password.title_with_name",
            "Passwort für Room {name}",
            {
                name: context.roomName
            }
        );
    }

    if (el.roomPasswordSubtitle)
    {
        el.roomPasswordSubtitle.textContent = tr(
            "room_password.subtitle.required",
            "Das Backend meldet: room password required"
        );
    }

    if (el.roomPasswordError)
    {
        el.roomPasswordError.textContent = "";
        el.roomPasswordError.style.display = "none";
    }

    if (el.roomPasswordInput)
    {
        el.roomPasswordInput.value = "";
    }

    el.roomPasswordModal.classList.add("visible");
    el.roomPasswordModal.setAttribute("aria-hidden", "false");

    setTimeout(function()
    {
        if (el.roomPasswordInput)
        {
            el.roomPasswordInput.focus();
        }
    }, 0);
}

async function saveRoomPassword(context, password)
{
    return await fetchJson("save_room_password.php",
    {
        method: "POST",
        headers:
        {
            "Content-Type": "application/json",
            "Accept": "application/json"
        },
        body: JSON.stringify(
        {
            room_node_id: context.roomNodeId,
            room_name: context.roomName,
            password: password
        })
    });
}

async function handleRoomPasswordSave()
{
    if (!state.roomPasswordPrompt || !el.roomPasswordInput)
    {
        return;
    }

    const password = el.roomPasswordInput.value.trim();

    if (password === "")
    {
        if (el.roomPasswordError)
        {
            el.roomPasswordError.textContent = tr(
                "room_password.error.empty",
                "Bitte ein Passwort eingeben."
            );
            el.roomPasswordError.style.display = "block";
        }

        el.roomPasswordInput.focus();
        return;
    }

    const context = state.roomPasswordPrompt;

    if (el.roomPasswordSaveButton)
    {
        el.roomPasswordSaveButton.disabled = true;
    }

    if (el.roomPasswordCancelButton)
    {
        el.roomPasswordCancelButton.disabled = true;
    }

    try
    {
        await saveRoomPassword(context, password);
        state.roomPasswordSuppressUntil.set(String(context.roomNodeId), Date.now() + 8000);
        closeRoomPasswordDialog();

        if (state.chatRow)
        {
            await loadChatMessages(state.chatRow, true);
        }
    }
    catch (error)
    {
        if (el.roomPasswordError)
        {
            el.roomPasswordError.textContent =
                error.message || tr("room_password.error.save_failed", "Speichern fehlgeschlagen.");
            el.roomPasswordError.style.display = "block";
        }
    }
    finally
    {
        if (el.roomPasswordSaveButton)
        {
            el.roomPasswordSaveButton.disabled = false;
        }

        if (el.roomPasswordCancelButton)
        {
            el.roomPasswordCancelButton.disabled = false;
        }
    }
}

function handleRoomPasswordRequired(txId, data)
{
    const roomName = data.room_name || (state.chatRow ? state.chatRow.name : "");
    const roomNodeId = Number(data.room_node_id || (state.chatRow ? state.chatRow.node_id : 0));

    if (!roomName || !roomNodeId)
    {
        stopTxPolling(txId);
        return;
    }

    if (isRoomPasswordPromptSuppressed(roomNodeId))
    {
        return;
    }

    if (
        state.roomPasswordPrompt &&
        state.roomPasswordPrompt.roomNodeId === roomNodeId
    )
    {
        return;
    }

    openRoomPasswordDialog(
    {
        txId: txId,
        roomName: roomName,
        roomNodeId: roomNodeId
    });
}

function startTxStatusPolling(txId)
{
    stopTxPolling(txId);

    const poll = async function()
    {
        try
        {
            const data = await fetchJson(`tx_status.php?id=${encodeURIComponent(txId)}`,
            {
                method: "GET",
                headers:
                {
                    "Accept": "application/json"
                }
            });

            if (data.state === "room_password_required")
            {
                handleRoomPasswordRequired(txId, data);
                return;
            }

            if (data.state === "missing" || data.state === "failed")
            {
                if (state.chatRow)
                {
                    await loadChatMessages(state.chatRow, true);
                }

                stopTxPolling(txId);
            }
        }
        catch (error)
        {
            console.error("TX-Status-Polling fehlgeschlagen:", error);
            stopTxPolling(txId);
        }
    };

    poll();

    const timer = setInterval(poll, 1500);
    state.txPollTimers.set(txId, timer);
}

function buildOutgoingPayload(row, messageText)
{
    if (isRoomNode(row))
    {
        return {
            tx_kind: 1,
            room_name: row.name || "",
            room_node_id: Number(row.node_id || 0),
            message_text: messageText,
            max_retries: 3
        };
    }

    if (isChannelRow(row))
    {
        return {
            tx_kind: 3,
            channel_name: row.name || "",
            channel_idx: Number(row.channel_idx || 0),
            message_text: messageText,
            max_retries: 3
        };
    }

    return {
        tx_kind: 0,
        target_name: row.name || "",
        target_node_id: Number(row.node_id || 0),
        message_text: messageText,
        max_retries: 3
    };
}

async function sendFloodAdvert()
{
    el.advertButton.disabled = true;

    try
    {
        const response = await fetch("send_message.php",
        {
            method: "POST",
            headers:
            {
                "Content-Type": "application/json",
                "Accept": "application/json"
            },
            body: JSON.stringify(
            {
                tx_kind: 2,
                message_text: "[flood advert]",
                max_retries: 1
            })
        });

        if (!response.ok)
        {
            throw new Error(`HTTP ${response.status}`);
        }

        const result = await response.json();

        if (!result.success)
        {
            throw new Error(result.error || "Unbekannter Fehler");
        }
    }
    catch (error)
    {
        alert("Advert senden fehlgeschlagen: " + error.message);
    }
    finally
    {
        el.advertButton.disabled = false;
    }
}

function setLeftTab(tabName)
{
    state.leftTab = tabName;

    const tabNodesBtn = document.getElementById("tabNodesBtn");
    const tabChannelsBtn = document.getElementById("tabChannelsBtn");
    const nodesPanel = document.getElementById("nodesPanel");
    const channelsPanel = document.getElementById("channelsPanel");

    if (tabNodesBtn)
    {
        tabNodesBtn.classList.toggle("active", tabName === "nodes");
    }

    if (tabChannelsBtn)
    {
        tabChannelsBtn.classList.toggle("active", tabName === "channels");
    }

    if (nodesPanel)
    {
        nodesPanel.classList.toggle("active", tabName === "nodes");
    }

    if (channelsPanel)
    {
        channelsPanel.classList.toggle("active", tabName === "channels");
    }

    updateLeftCounter();
}

function renderChannelsList()
{
    const listEl = document.getElementById("channelsList");

    if (!listEl)
    {
        return;
    }

    listEl.innerHTML = "";

    if (!Array.isArray(state.channels) || state.channels.length === 0)
    {
        listEl.innerHTML =
            "<div class=\"channels-empty\">" +
            escapeHtml(tr("channels.none", "Keine Channels vorhanden")) +
            "</div>";
        return;
    }

    state.channels.sort(function(a, b)
    {
        if (a.is_default) return -1;
        if (b.is_default) return 1;
        return a.name.localeCompare(b.name);
    });

    state.channels.forEach(function(channel)
    {
        const btn = document.createElement("button");
        btn.type = "button";
        btn.className = "channel-item";

        const channelIdx = Number(channel.channel_idx || 0);
        const channelName =
            (channel.name && String(channel.name).trim() !== "")
                ? String(channel.name)
                : (tr("channel.fallback_name", "Channel {idx}", { idx: channelIdx }));

        if (
            state.chatRow &&
            state.chatRow.type === "channel" &&
            Number(state.chatRow.channel_idx || 0) === channelIdx
        )
        {
            btn.classList.add("active");
        }

        const titleEl = document.createElement("span");
        titleEl.textContent = channelName;
        btn.appendChild(titleEl);

        const metaParts = [];
        metaParts.push("IDX " + String(channelIdx));

        if (channel.is_default)
        {
            metaParts.push(tr("channel.meta.default", "default"));
        }

        if (channel.is_observed)
        {
            metaParts.push(tr("channel.meta.observed", "observed"));
        }

        if (!channel.enabled)
        {
            metaParts.push(tr("channel.meta.disabled", "disabled"));
        }

        const metaEl = document.createElement("span");
        metaEl.className = "channel-item-meta";
        metaEl.textContent = metaParts.join(" • ");
        btn.appendChild(metaEl);

        btn.addEventListener("click", function()
        {
            state.chatRow =
            {
                type: "channel",
                name: channelName,
                channel_idx: channelIdx,
                enabled: !!channel.enabled,
                is_observed: !!channel.is_observed,
                is_default: !!channel.is_default
            };

            setLeftTab("channels");
            showChatForRow(state.chatRow);
            renderChannelsList();
        });

        listEl.appendChild(btn);
    });
}

async function loadChannels()
{
    const data = await fetchJson("channels.php",
    {
        method: "GET",
        cache: "no-store",
        headers:
        {
            "Accept": "application/json"
        }
    });

    state.channels = Array.isArray(data.channels) ? data.channels : [];

    updateLeftCounter();
}

async function sendCurrentChatMessage()
{
    if (!state.chatRow || !el.chatInput)
    {
        return;
    }

    const messageText = el.chatInput.value.trim();

    if (messageText === "")
    {
        return;
    }

    if (!isChatLikeNode(state.chatRow))
    {
        return;
    }

    if (isChannelRow(state.chatRow))
    {
        if (!state.chatRow.enabled)
        {
            window.alert(tr("channel.send.disabled", "Dieser Channel ist deaktiviert."));
            return;
        }

        if (state.chatRow.is_observed)
        {
            window.alert(
                tr(
                    "channel.send.observed_no_context",
                    "Für diesen beobachteten Channel ist kein lokaler Sendekontext konfiguriert."
                )
            );
            return;
        }
    }

    if ((state.chatRow.name || "") === "")
    {
        return;
    }

    setChatInputEnabled(false);

    try
    {
        const data = await fetchJson("send_message.php",
        {
            method: "POST",
            headers:
            {
                "Content-Type": "application/json",
                "Accept": "application/json"
            },
            body: JSON.stringify(buildOutgoingPayload(state.chatRow, messageText))
        });

        el.chatInput.value = "";
        await loadChatMessages(state.chatRow, false);

        if (data.id)
        {
            startTxStatusPolling(data.id);
        }
    }
    catch (error)
    {
        window.alert(
            tr("chat.send_error", "Fehler beim Senden: {message}",
            {
                message: error.message || tr("error.unknown", "Unbekannter Fehler")
            })
        );
    }
    finally
    {
        setChatInputEnabled(true);
        el.chatInput.focus();
        scrollChatToBottom();
    }
}

initLeftTabs();

table = new Tabulator("#nodesTable",
{
    layout: "fitColumns",
    height: "100%",
    ajaxURL: "nodes.php",
    ajaxConfig:
    {
        method: "GET"
    },
    ajaxParams: function()
    {
        return {
            type: el.typeFilter.value
        };
    },
    rowFormatter: function(row)
    {
        const data = row.getData();

        if (data.msg_count > 0 && isChatLikeNode(data))
        {
            row.getElement().classList.add("has-messages");
        }
    },
    ajaxContentType: "json",
    ajaxLoader: false,
    progressiveLoad: false,
    placeholder: tr("table.placeholder.no_nodes", "Keine Nodes gefunden."),
    index: "id",
    initialSort:
    [
        { column: "last_advert_at", dir: "desc" }
    ],
    columns:
    [
        {
            title: tr("table.column.type", "Type"),
            field: "advert_type_label",
            width: 120,
            hozAlign: "left",
            formatter: advertTypeFormatter,
            headerSort: true
        },
        {
            title: tr("table.column.name", "Name"),
            field: "name",
            sorter: "string",
            headerSort: true,
            formatter: nameFormatter,
            cellClick: function(e, cell)
            {
                const row = cell.getRow().getData();

                if (isChatLikeNode(row))
                {
                    showChatForRow(row);
                }
                else
                {
                    showInfoForRow(row);
                }
            },
            cellContext: function(e, cell)
            {
                e.preventDefault();

                const row = cell.getRow().getData();

                if (hasLocation(row))
                {
                    state.autoZoom = true;
                    showMapForRow(row);
                }
                else if (isChatLikeNode(row))
                {
                    showChatForRow(row);
                }
                else
                {
                    showInfoForRow(row);
                }
            }
        },
        {
            title: tr("table.column.last", "Last"),
            field: "last_advert_at",
            width: 90,
            hozAlign: "left",
            formatter: relativeTime,
            tooltip: true,
            sorter: lastAdvertSorter,
            headerSort: true
        }
    ],
    ajaxResponse: function(url, params, response)
    {
        if (!response.success)
        {
            throw new Error(response.error || tr("error.unknown", "Unbekannter Fehler"));
        }

        const nodes = response.nodes || [];

        return nodes.filter(function(node)
        {
            if (!node.last_advert_at)
            {
                return false;
            }

            const timestamp = parseMariaDbDateTime(node.last_advert_at);

            if (!timestamp || timestamp <= 0)
            {
                return false;
            }

            const diff = Math.floor((Date.now() - timestamp) / 1000);

            if (diff <= 0)
            {
                return false;
            }

            if (el.callsignFilter && el.callsignFilter.checked)
            {
                if (!containsPossibleCallsign(node.name))
                {
                    return false;
                }
            }

            return true;
        });
    },
    ajaxError: function(xhr, textStatus, errorThrown)
    {
        const message = errorThrown || textStatus || tr("error.http", "HTTP Fehler");
        el.tableError.innerHTML =
            `<div class="error-box">${escapeHtml(tr("error.loading", "Fehler beim Laden: {message}", { message: message }))}</div>`;
        el.nodeCount.textContent = tr("status.error", "Fehler");
    }
});

async function saveChannel(payload)
{
    return await fetchJson("save_channel.php",
    {
        method: "POST",
        headers:
        {
            "Content-Type": "application/json",
            "Accept": "application/json"
        },
        body: JSON.stringify(payload)
    });
}

async function deleteChannel(channelIdx)
{
    return await fetchJson("delete_channel.php",
    {
        method: "POST",
        headers:
        {
            "Content-Type": "application/json",
            "Accept": "application/json"
        },
        body: JSON.stringify(
        {
            channel_idx: Number(channelIdx || 0)
        })
    });
}

function buildChannelDialogPayload()
{
    if (!state.channelDialog)
    {
        return null;
    }

    const action = state.channelDialog.action;
    const rawName = el.channelNameInput ? el.channelNameInput.value.trim() : "";
    const rawSecret = el.channelSecretInput ? el.channelSecretInput.value.trim() : "";

    switch (action)
    {
        case "create_private":
            return {
                action: "create_private",
                name: rawName
            };

        case "join_private":
            return {
                action: "join_private",
                name: rawName,
                secret_key: rawSecret
            };

        case "join_public":
            return {
                action: "join_public",
                name: rawName
            };

        case "join_hashtag":
            return {
                action: "join_hashtag",
                name: rawName
            };

        default:
            return null;
    }
}

async function handleChannelDialogConfirm()
{
    if (!state.channelDialog)
    {
        return;
    }

    if (state.channelDialog.action === "done_after_create")
    {
        closeChannelDialog();
        return;
    }

    const action = state.channelDialog.action;

    if (el.channelModalError)
    {
        el.channelModalError.textContent = "";
        el.channelModalError.style.display = "none";
    }

    if (el.channelModalConfirmButton)
    {
        el.channelModalConfirmButton.disabled = true;
    }

    if (el.channelModalCancelButton)
    {
        el.channelModalCancelButton.disabled = true;
    }

    try
    {
        if (action === "remove")
        {
            const selected = getSelectedChannel();

            if (!selected)
            {
                throw new Error(tr("channel.none_selected", "Kein Channel ausgewählt."));
            }

            await deleteChannel(selected.channel_idx);

            if (
                state.chatRow &&
                isChannelRow(state.chatRow) &&
                Number(state.chatRow.channel_idx || 0) === Number(selected.channel_idx || 0)
            )
            {
                showEmptyRightPanel();
            }

            await loadChannels();
            renderChannelsList();
            closeChannelDialog();
            return;
        }

        const payload = buildChannelDialogPayload();

        if (!payload)
        {
            throw new Error(tr("channel.invalid_action", "Ungültige Aktion."));
        }

        const result = await saveChannel(payload);

        await loadChannels();
        renderChannelsList();

        if (result.channel)
        {
            state.chatRow =
            {
                type: "channel",
                name: result.channel.name || ("Channel " + String(result.channel.channel_idx || "")),
                channel_idx: Number(result.channel.channel_idx || 0),
                enabled: !!result.channel.enabled,
                is_observed: !!result.channel.is_observed,
                is_default: !!result.channel.is_default
            };

            setLeftTab("channels");
            await showChatForRow(state.chatRow);
            renderChannelsList();
        }

        if (action === "create_private" && result.secret_key)
        {
            if (el.channelResultSecret)
            {
                el.channelResultSecret.value = result.secret_key;
            }

            if (el.channelResultGroup)
            {
                el.channelResultGroup.style.display = "";
            }

            if (el.channelModalSubtitle)
            {
                el.channelModalSubtitle.textContent = tr(
                    "channel.action.create_private.done_subtitle",
                    "Channel wurde erstellt. Diesen Secret Key jetzt mit den anderen Teilnehmern teilen."
                );
            }

            const qrPayload = buildMeshCoreChannelQrPayload(
                result.channel?.name || payload.name || "",
                result.secret_key
            );

            renderChannelQrCode(qrPayload);

            if (el.channelModalConfirmButton)
            {
                el.channelModalConfirmButton.textContent = tr("common.done", "Done");
                el.channelModalConfirmButton.disabled = false;
            }

            if (el.channelModalCancelButton)
            {
                el.channelModalCancelButton.style.display = "none";
                el.channelModalCancelButton.disabled = true;
            }

            state.channelDialog.action = "done_after_create";
            return;
        }

        closeChannelDialog();
    }
    catch (error)
    {
        showChannelDialogError(error.message || tr("error.generic", "Fehler"));
    }
    finally
    {
        if (state.channelDialog && state.channelDialog.action !== "done_after_create")
        {
            if (el.channelModalConfirmButton)
            {
                el.channelModalConfirmButton.disabled = false;
            }

            if (el.channelModalCancelButton)
            {
                el.channelModalCancelButton.disabled = false;
            }
        }
    }
}

function updateLeftCounter()
{
    if (!el.nodeCount)
    {
        return;
    }

    if (state.leftTab === "channels")
    {
        const channels = Array.isArray(state.channels) ? state.channels : [];
        const count = channels.length;

        const observedCount = channels.filter(function(channel)
        {
            return !!channel.is_observed;
        }).length;

        if (observedCount > 0)
        {
            el.nodeCount.textContent = tr(
                "counter.channels_observed",
                "{count} Channels ({observed} observed)",
                {
                    count: count,
                    observed: observedCount
                }
            );
        }
        else
        {
            el.nodeCount.textContent = tr(
                "counter.channels",
                "{count} Channels",
                {
                    count: count
                }
            );
        }

        return;
    }

    if (typeof table === "undefined" || !table)
    {
        el.nodeCount.textContent = tr("counter.nodes", "0 Nodes", { count: 0 });
        return;
    }

    const rows = table.getData();
    const count = Array.isArray(rows) ? rows.length : 0;

    el.nodeCount.textContent = tr(
        "counter.nodes",
        "{count} Nodes",
        {
            count: count
        }
    );
}

function updatePageTitle(nodeName)
{
    const baseTitle = tr("page.title", "MeshCore Web-Dashboard");

    if (el.pageTitle)
    {
        el.pageTitle.textContent = nodeName
            ? baseTitle + " – " + nodeName
            : baseTitle;
    }

    document.title = nodeName
        ? "MeshCore – " + nodeName
        : tr("document.title", "MeshCore Dashboard");
}

function initLeftTabs()
{
    const tabNodesBtn = document.getElementById("tabNodesBtn");
    const tabChannelsBtn = document.getElementById("tabChannelsBtn");

    if (tabNodesBtn)
    {
        tabNodesBtn.addEventListener("click", function()
        {
            setLeftTab("nodes");
        });
    }

    if (tabChannelsBtn)
    {
        tabChannelsBtn.addEventListener("click", function()
        {
            setLeftTab("channels");
        });
    }

    setLeftTab("nodes");
}

async function openSetupDialog()
{
    if (!el.setupModal)
    {
        return;
    }

    if (el.setupModalError)
    {
        el.setupModalError.textContent = "";
        el.setupModalError.style.display = "none";
    }

    if (el.setupNameInput)
    {
        el.setupNameInput.value = "";
    }

    if (el.setupLatInput)
    {
        el.setupLatInput.value = "";
    }

    if (el.setupLonInput)
    {
        el.setupLonInput.value = "";
    }

    try
    {
        const data = await loadCompanionSetup();
        const cfg = data.config;

        if (cfg)
        {
            if (el.setupNameInput)
            {
                el.setupNameInput.value = cfg.name || "";
            }

            if (el.setupLatInput)
            {
                el.setupLatInput.value =
                    Number.isFinite(Number(cfg.latitude))
                        ? String(cfg.latitude)
                        : "";
            }

            if (el.setupLonInput)
            {
                el.setupLonInput.value =
                    Number.isFinite(Number(cfg.longitude))
                        ? String(cfg.longitude)
                        : "";
            }
        }
    }
    catch (error)
    {
        if (el.setupModalError)
        {
            el.setupModalError.textContent =
                error.message || tr("setup.error.load_failed", "Setup-Werte konnten nicht geladen werden.");
            el.setupModalError.style.display = "block";
        }
    }

    el.setupModal.classList.add("visible");
    el.setupModal.setAttribute("aria-hidden", "false");

    if (el.setupNameInput)
    {
        el.setupNameInput.focus();
    }
}

async function refreshNodesTableKeepScroll()
{
    const holder = table.rowManager && table.rowManager.element
        ? table.rowManager.element
        : null;

    const scrollTop = holder ? holder.scrollTop : 0;

    await table.replaceData();

    if (holder)
    {
        holder.scrollTop = scrollTop;
    }
}

function closeSetupDialog()
{
    if (!el.setupModal)
    {
        return;
    }

    el.setupModal.classList.remove("visible");
    el.setupModal.setAttribute("aria-hidden", "true");

    if (el.setupModalError)
    {
        el.setupModalError.textContent = "";
        el.setupModalError.style.display = "none";
    }
}

async function loadCompanionSetup()
{
    return await fetchJson("companion_setup_read.php",
    {
        method: "GET",
        cache: "no-store",
        headers:
        {
            "Accept": "application/json"
        }
    });
}

async function applyCompanionSetup()
{
    const name = el.setupNameInput ? el.setupNameInput.value.trim() : "";
    const latText = el.setupLatInput ? el.setupLatInput.value.trim() : "";
    const lonText = el.setupLonInput ? el.setupLonInput.value.trim() : "";

    const latitude = Number(latText);
    const longitude = Number(lonText);

    if (name === "")
    {
        throw new Error(tr("setup.error.name_missing", "Name fehlt."));
    }

    if (!Number.isFinite(latitude) || latitude < -90 || latitude > 90)
    {
        throw new Error(tr("setup.error.latitude_invalid", "Latitude ist ungültig."));
    }

    if (!Number.isFinite(longitude) || longitude < -180 || longitude > 180)
    {
        throw new Error(tr("setup.error.longitude_invalid", "Longitude ist ungültig."));
    }

    return await fetchJson("companion_setup.php",
    {
        method: "POST",
        headers:
        {
            "Content-Type": "application/json",
            "Accept": "application/json"
        },
        body: JSON.stringify(
        {
            name: name,
            latitude: latitude,
            longitude: longitude
        })
    });
}

if (el.setupButton)
{
    el.setupButton.addEventListener("click", async function()
    {
        await openSetupDialog();
    });
}

if (el.setupCancelButton)
{
    el.setupCancelButton.addEventListener("click", function()
    {
        closeSetupDialog();
    });
}

if (el.setupApplyButton)
{
    el.setupApplyButton.addEventListener("click", async function()
    {
        try
        {
            await applyCompanionSetup();
            updatePageTitle(el.setupNameInput ? el.setupNameInput.value.trim() : "");
            closeSetupDialog();
        }
        catch (error)
        {
            if (el.setupModalError)
            {
                el.setupModalError.textContent =
                    error.message || tr("setup.error.apply_failed", "Setup fehlgeschlagen.");
                el.setupModalError.style.display = "block";
            }
        }
    });
}

table.on("dataProcessed", function()
{
    updateLeftCounter();
    el.tableError.innerHTML = "";

    if (state.rightView === "allmap")
    {
        showAllNodesMap();
    }
});

if (el.typeFilter)
{
    el.typeFilter.addEventListener("change", function()
    {
        el.tableError.innerHTML = "";
        table.replaceData();
    });
}

if (el.callsignFilter)
{
    el.callsignFilter.addEventListener("change", function()
    {
        el.tableError.innerHTML = "";
        table.replaceData();
    });
}

if (el.allMapButton)
{
    el.allMapButton.addEventListener("click", function()
    {
        state.autoZoom = true;
        showAllNodesMap();
    });
}

if (el.chatInput)
{
    el.chatInput.addEventListener("keydown", function(e)
    {
        if (e.key === "Enter" && !e.shiftKey)
        {
            e.preventDefault();
            sendCurrentChatMessage();
        }
    });
}

if (el.chatSendButton)
{
    el.chatSendButton.addEventListener("click", function()
    {
        sendCurrentChatMessage();
    });
}

if (el.roomPasswordSaveButton)
{
    el.roomPasswordSaveButton.addEventListener("click", function()
    {
        handleRoomPasswordSave();
    });
}

if (el.roomPasswordCancelButton)
{
    el.roomPasswordCancelButton.addEventListener("click", function()
    {
        closeRoomPasswordDialog();
    });
}

if (el.roomPasswordInput)
{
    el.roomPasswordInput.addEventListener("keydown", function(e)
    {
        if (e.key === "Enter")
        {
            e.preventDefault();
            handleRoomPasswordSave();
        }
        else if (e.key === "Escape")
        {
            e.preventDefault();
            closeRoomPasswordDialog();
        }
    });
}

if (el.roomPasswordModal)
{
    el.roomPasswordModal.addEventListener("click", function(e)
    {
        if (e.target === el.roomPasswordModal)
        {
            closeRoomPasswordDialog();
        }
    });
}

if (el.advertButton)
{
    el.advertButton.addEventListener("click", sendFloodAdvert);
}

if (el.createPrivateChannelBtn)
{
    el.createPrivateChannelBtn.addEventListener("click", function()
    {
        openChannelDialog("create_private");
    });
}

if (el.joinPrivateChannelBtn)
{
    el.joinPrivateChannelBtn.addEventListener("click", function()
    {
        openChannelDialog("join_private");
    });
}

if (el.joinPublicChannelBtn)
{
    el.joinPublicChannelBtn.addEventListener("click", function()
    {
        openChannelDialog("join_public");
    });
}

if (el.joinHashtagChannelBtn)
{
    el.joinHashtagChannelBtn.addEventListener("click", function()
    {
        openChannelDialog("join_hashtag");
    });
}

if (el.removeChannelBtn)
{
    el.removeChannelBtn.addEventListener("click", function()
    {
        openChannelDialog("remove");
    });
}

if (el.channelModalConfirmButton)
{
    el.channelModalConfirmButton.addEventListener("click", function()
    {
        handleChannelDialogConfirm();
    });
}

if (el.channelModalCancelButton)
{
    el.channelModalCancelButton.addEventListener("click", function()
    {
        closeChannelDialog();
    });
}

if (el.channelModal)
{
    el.channelModal.addEventListener("click", function(e)
    {
        if (e.target === el.channelModal)
        {
            closeChannelDialog();
        }
    });
}

if (el.channelNameInput)
{
    el.channelNameInput.addEventListener("keydown", function(e)
    {
        if (e.key === "Enter")
        {
            e.preventDefault();
            handleChannelDialogConfirm();
        }
        else if (e.key === "Escape")
        {
            e.preventDefault();
            closeChannelDialog();
        }
    });
}

if (el.channelSecretInput)
{
    el.channelSecretInput.addEventListener("keydown", function(e)
    {
        if (e.key === "Enter")
        {
            e.preventDefault();
            handleChannelDialogConfirm();
        }
        else if (e.key === "Escape")
        {
            e.preventDefault();
            closeChannelDialog();
        }
    });
}

if (el.discoverButton)
{
    el.discoverButton.addEventListener("click", async function()
    {
        await openDiscoverDialog();
    });
}

if (el.discoverRepeatButton)
{
    el.discoverRepeatButton.addEventListener("click", handleDiscoverStartClick);
}

if (el.discoverCloseButton)
{
    el.discoverCloseButton.addEventListener("click", async function()
    {
        await closeDiscoverDialog();
    });
}

setChatInputEnabled(false);
showEmptyRightPanel();

(async function()
{
    try
    {
        await loadChannels();
        renderChannelsList();
    }
    catch (err)
    {
        console.error("Initial channels load failed:", err);
    }
})();

(async function()
{
    try
    {
        const data = await loadCompanionSetup();
        const cfg = data && data.config ? data.config : null;
        updatePageTitle(cfg && cfg.name ? cfg.name : "");
    }
    catch (err)
    {
        console.error("Initial setup load failed:", err);
        updatePageTitle("");
    }
})();

setInterval(function()
{
    refreshNodesTableKeepScroll();
}, 5000);

setInterval(async function()
{
    try
    {
        await loadChannels();
        renderChannelsList();
    }
    catch (err)
    {
        console.error("Channels refresh failed:", err);
    }
}, 10000);


