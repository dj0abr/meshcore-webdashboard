let table = null;

const CONTACT_READ_STORAGE_KEY = "meshcore.contactLastReadEpoch";
const PATH_DISPLAY_SETTINGS_STORAGE_KEY = "meshcore.pathDisplaySettings";
const NOTIFICATION_AUDIO_ENABLED_STORAGE_KEY = "meshcore.notificationAudioEnabled";
const MONITOR_VISIBLE_COLUMNS_STORAGE_KEY = "meshcore.monitorVisibleColumns.v2";
const MONITOR_COLUMN_WIDTHS_STORAGE_KEY = "meshcore.monitorColumnWidths.v1";
const MONITOR_REFRESH_INTERVAL_MS = 2000;

const MeshCoreShared = window.MeshCoreShared;
const MeshCoreApi = window.MeshCoreApi;

const
{
    escapeHtml,
    isChatNode,
    isRoomNode,
    isChannelRow,
    isChatLikeNode,
    isRepeaterNode,
    getChatKindLabel,
    getChatKindValue,
    parseMariaDbDateTime,
    containsPossibleCallsign,
    getNodeLatLon,
    hasLocation,
    hasValidCoords,
    hasValidEndpointCoords,
    e6ToDegrees,
    degToRad,
    distanceMeters,
    formatDistanceMeters,
    loadNotificationAudioEnabled,
    loadPathDisplaySettings,
    formatMessageText,
    normalizeGuiTimestamp,
    getTextCharacters,
    limitTextCharacters,
    getLocale,
    tr,
    buildMeshCoreChannelQrPayload,
    formatDateTime,
    formatEpochDateTime,
    loadContactReadState,
    saveContactReadState,
    getContactReadKey,
    getContactLastReadEpoch,
    markContactAsRead,
    getOutgoingStatusClass,
    extractReplyNameFromMessage,
    pruneImplausibleResolvedHops,
    resolvePathGreedyFromEndpoint,
    resolvePathBestRoute,
    buildResolvedPathListEntry,
    pathHasNoGaps,
    selectPreferredResolvedPath,
    buildResolvedPathList,
    loadChannelReadState,
    saveChannelReadState,
    getChannelReadKey,
    getChannelLastReadEpoch,
    markChannelAsRead
} = MeshCoreShared;


function saveNotificationAudioEnabled()
{
    try
    {
        localStorage.setItem(
            NOTIFICATION_AUDIO_ENABLED_STORAGE_KEY,
            state.notificationAudioEnabled ? "1" : "0"
        );
    }
    catch (err)
    {
        // localStorage may be disabled/private. Ignore and keep current session state.
    }
}

function syncNotificationAudioCheckbox()
{
    if (el.notificationAudioToggle)
    {
        el.notificationAudioToggle.checked = state.notificationAudioEnabled;
    }
}

function savePathDisplaySettings()
{
    try
    {
        localStorage.setItem(
            PATH_DISPLAY_SETTINGS_STORAGE_KEY,
            JSON.stringify(
            {
                showNames: !!state.chatPathShowNames,
                showHash: !!state.chatPathShowHash,
                showDistances: !!state.chatPathShowDistances
            })
        );
    }
    catch (err)
    {
        // localStorage may be disabled/private. Ignore and keep current session state.
    }
}

function syncPathDisplayCheckboxes()
{
    if (el.chatPathToggleNames)
    {
        el.chatPathToggleNames.checked = state.chatPathShowNames;
    }

    if (el.chatPathToggleHash)
    {
        el.chatPathToggleHash.checked = state.chatPathShowHash;
    }

    if (el.chatPathToggleDistances)
    {
        el.chatPathToggleDistances.checked = state.chatPathShowDistances;
    }

    if (el.mapPathToggleNames)
    {
        el.mapPathToggleNames.checked = state.chatPathShowNames;
    }

    if (el.mapPathToggleHash)
    {
        el.mapPathToggleHash.checked = state.chatPathShowHash;
    }

    if (el.mapPathToggleDistances)
    {
        el.mapPathToggleDistances.checked = state.chatPathShowDistances;
    }
}

const pathDisplaySettings = loadPathDisplaySettings();

const el =
{
    nodeCount: document.getElementById("nodeCount"),
    typeFilter: document.getElementById("typeFilter"),
    tableError: document.getElementById("tableError"),
    mapEmpty: document.getElementById("mapEmpty"),
    chatView: document.getElementById("chatView"),
    chatTitle: document.getElementById("chatTitle"),
    callsignFilter: document.getElementById("callsignFilter"),
    activeFilter: document.getElementById("activeFilter"),
    //localFilter: document.getElementById("localFilter"),
    chatBody: document.getElementById("chatBody"),
    chatTabsView: document.getElementById("chatTabsView"),
    tabMessagesViewBtn: document.getElementById("tabMessagesViewBtn"),
    tabPathMapViewBtn: document.getElementById("tabPathMapViewBtn"),
    notificationAudioToggle: document.getElementById("notificationAudioToggle"),
    messagesTabView: document.getElementById("messagesTabView"),
    pathMapTabView: document.getElementById("pathMapTabView"),
    allMapButton: document.getElementById("allMapButton"),
    mapViewWrapper: document.getElementById("mapViewWrapper"),
    mapPathControls: document.getElementById("mapPathControls"),
    mapPathToggleNames: document.getElementById("mapPathToggleNames"),
    mapPathToggleHash: document.getElementById("mapPathToggleHash"),
    mapPathToggleDistances: document.getElementById("mapPathToggleDistances"),
    mapView: document.getElementById("mapView"),
    chatInput: document.getElementById("chatInput"),
    chatSymbolButton: document.getElementById("chatSymbolButton"),
    chatSymbolPalette: document.getElementById("chatSymbolPalette"),
    chatSendButton: document.getElementById("chatSendButton"),
    messagesPanelContent: document.getElementById("messagesPanelContent"),
    chatPathMapPanel: document.getElementById("chatPathMapPanel"),
    chatPathMap: document.getElementById("chatPathMap"),
    chatPathMapInfo: document.getElementById("chatPathMapInfo"),
    chatPathToggleNames: document.getElementById("chatPathToggleNames"),
    chatPathToggleHash: document.getElementById("chatPathToggleHash"),
    chatPathToggleDistances: document.getElementById("chatPathToggleDistances"),
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
    tabMonitorBtn: document.getElementById("tabMonitorBtn"),
    monitorView: document.getElementById("monitorView"),
    monitorColumnToolbar: document.getElementById("monitorColumnToolbar"),
    monitorTable: document.getElementById("monitorTable"),
    channelsList: document.getElementById("channelsList"),
    channelActionSelect: document.getElementById("channelActionSelect"),
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
    protectedRepeaterButton: document.getElementById("protectedRepeaterButton"),
    protectedRepeaterModal: document.getElementById("protectedRepeaterModal"),
    protectedRepeaterStatusText: document.getElementById("protectedRepeaterStatusText"),
    protectedRepeaterNameInput: document.getElementById("protectedRepeaterNameInput"),
    protectedRepeaterResult: document.getElementById("protectedRepeaterResult"),
    protectedRepeaterStartButton: document.getElementById("protectedRepeaterStartButton"),
    protectedRepeaterCloseButton: document.getElementById("protectedRepeaterCloseButton"),
    protectedRepeaterModalError: document.getElementById("protectedRepeaterModalError"),
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
    setupCityInput: document.getElementById("setupCityInput"),
    setupLatInput: document.getElementById("setupLatInput"),
    setupLonInput: document.getElementById("setupLonInput"),
    setupBotInput: document.getElementById("setupBotInput"),
    setupApplyButton: document.getElementById("setupApplyButton"),
    setupCancelButton: document.getElementById("setupCancelButton"),
    setupModalError: document.getElementById("setupModalError"),
    pageTitle: document.getElementById("pageTitle"),
    rightPanelTitle: document.getElementById("rightPanelTitle"),
    rightPanelSubtitle: document.getElementById("rightPanelSubtitle"),
    rightPanelActions: document.getElementById("rightPanelActions"),
    mapPositionButton: document.getElementById("mapPositionButton"),
    mapPathButton: document.getElementById("mapPathButton"),
    resetPathButton: document.getElementById("resetPathButton"),
    noiseFloorMeter: document.getElementById("noiseFloorMeter"),
    noiseFloorFill: document.getElementById("noiseFloorFill"),
    noiseFloorText: document.getElementById("noiseFloorText"),
    dataRateMeter: document.getElementById("dataRateMeter"),
    dataRateFill: document.getElementById("dataRateFill"),
    dataRateText: document.getElementById("dataRateText"),
    batteryVoltageText: document.getElementById("batteryVoltageText"),
    companionLinkLed: document.getElementById("companionLinkLed"),
};

const state =
{
    leafletMap: null,
    leafletMarkers: null,
    chatPathLeafletMap: null,
    chatPathLayer: null,
    chatPathShowNames: pathDisplaySettings.showNames,
    chatPathShowHash: pathDisplaySettings.showHash,
    chatPathShowDistances: pathDisplaySettings.showDistances,
    chatPathLastCorrelationKey: "",
    chatPathLastPreferredPath: null,
    lastNodeAdvertPathRow: null,
    autoZoom: true,
    rightView: "empty",
    chatRow: null,
    chatRefreshTimer: null,
    chatLastMessageId: 0,
    chatMessages: [],
    incomingMessagePollTimer: null,
    incomingMessageLastId: 0,
    incomingMessagePollActive: false,
    incomingMessageDetectionReady: false,
    lastIncomingMessageId: 0,
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
    protectedRepeaterPollTimer: null,
    protectedRepeaterModalOpen: false,
    protectedRepeaterPending: false,
    protectedRepeaterActionId: null,
    rightTab: "messages",
    rightPanelMode: "messages",
    monitorTable: null,
    monitorRows: [],
    monitorVisibleColumns: null,
    monitorColumnWidths: null,
    monitorRefreshTimer: null,
    monitorRefreshBusy: false,
    monitorLastId: 0,
    mapContextRow: null,
    resetPathPending: false,
    noiseFloorRefreshTimer: null,
    notificationAudioEnabled: loadNotificationAudioEnabled(),
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

const DEBUG_ENABLED = false;

const CHANNEL_READ_STORAGE_KEY = "meshcore.channelLastReadEpoch";

function consoledebug()
{
    if (!DEBUG_ENABLED)
    {
        return;
    }

    console.debug(...arguments);
}

function showRepeaterInfo(row)
{
    stopCurrentChatRefresh();
    resetChatState();
    state.rightView = "info";
    setRightPanelMode("messages", null);

    setChatInputEnabled(false);
    hideRightPanelViews();
    hideChatPathMap();

    const pos = getNodeLatLon(row);

    const nodeIdText =
        row.node_id !== null && row.node_id !== undefined
            ? String(row.node_id)
            : "-";

    const prefix6Text =
        String(row.prefix6_hex || "").trim() !== ""
            ? String(row.prefix6_hex)
            : "-";

    const publicKeyText =
        String(row.public_key_hex || "").trim() !== ""
            ? String(row.public_key_hex)
            : "-";

    const lastAdvertText =
        row.last_advert_at
            ? formatDateTime(row.last_advert_at)
            : "-";

    const firstSeenText =
        row.first_seen_at
            ? formatDateTime(row.first_seen_at)
            : "-";

    const updatedAtText =
        row.updated_at
            ? formatDateTime(row.updated_at)
            : "-";

    const lastModText =
        row.last_mod_at
            ? formatDateTime(row.last_mod_at)
            : "-";

    const advertFlagsText =
        row.advert_flags !== null && row.advert_flags !== undefined
            ? String(row.advert_flags)
            : "-";

    const positionText = pos
        ? `${pos.lat.toFixed(6)}, ${pos.lon.toFixed(6)}`
        : tr("map.no_position_for_node", "keine Positionsdaten verfügbar.");

    if (el.mapEmpty)
    {
        el.mapEmpty.innerHTML = `
            <div style="display:flex; flex-direction:column; gap:10px; width:100%; max-width:900px; justify-self:start; text-align:left;">
                <div>
                    📡 <strong>${escapeHtml(row.name || "-")}</strong>
                </div>

                <div style="display:grid; grid-template-columns:max-content 1fr; gap:8px 14px; align-items:start;">
                    <strong>${escapeHtml(tr("toolbar.type_short", "Typ"))}</strong><span>${escapeHtml(row.advert_type_label || "REPEATER")}</span>
                    <strong>${escapeHtml(tr("node.id", "Node-ID"))}</strong><span>${escapeHtml(nodeIdText)}</span>
                    <strong>${escapeHtml(tr("node.prefix6", "Prefix6"))}</strong><span>${escapeHtml(prefix6Text)}</span>
                    <strong>${escapeHtml(tr("node.public_key", "Public Key"))}</strong><span style="word-break:break-all;">${escapeHtml(publicKeyText)}</span>
                    <strong>${escapeHtml(tr("repeater.last_advert", "Letztes Advert"))}</strong><span>${escapeHtml(lastAdvertText)}</span>
                    <strong>${escapeHtml(tr("repeater.first_seen", "Erstmals gesehen"))}</strong><span>${escapeHtml(firstSeenText)}</span>
                    <strong>${escapeHtml(tr("repeater.updated_at", "Zuletzt aktualisiert"))}</strong><span>${escapeHtml(updatedAtText)}</span>
                    <strong>${escapeHtml(tr("repeater.last_mod", "Last Mod"))}</strong><span>${escapeHtml(lastModText)}</span>
                    <strong>${escapeHtml(tr("repeater.advert_flags", "Advert Flags"))}</strong><span>${escapeHtml(advertFlagsText)}</span>
                    <strong>${escapeHtml(tr("map.position", "Position"))}</strong><span>${escapeHtml(positionText)}</span>
                </div>
            </div>
        `;

        el.mapEmpty.style.display = "grid";
        el.mapEmpty.style.justifyItems = "start";
        el.mapEmpty.style.alignItems = "start";
    }
}

function renderBatteryVoltage(batteryMv, updatedAt)
{
    if (!el.batteryVoltageText)
    {
        return;
    }

    if (!Number.isFinite(batteryMv))
    {
        el.batteryVoltageText.textContent = `${tr("radio.battery_short", "Batt")}: --`;
        el.batteryVoltageText.title = tr("radio.battery_no_value", "Battery: kein Wert");

        return;
    }

    const voltage = batteryMv / 1000.0;
    const voltageText = voltage.toFixed(3).replace(".", ",");

    el.batteryVoltageText.textContent = `${tr("radio.battery_short", "Batt")}: ${voltageText} V`;
    el.batteryVoltageText.title =
        `${tr("radio.battery", "Battery")}: ${voltage.toFixed(3)} V` +
        (updatedAt ? `\n${tr("common.update", "Update")}: ${updatedAt}` : "");
}


const DATA_RATE_CONFIG =
{
    maxBps: 62500
};

function dataRatePercent(bps)
{
    const maxBps = DATA_RATE_CONFIG.maxBps;
    const clamped = Math.max(0, Math.min(maxBps, bps));

    if (clamped <= 0)
    {
        return 0;
    }

    return (Math.log10(clamped + 1) / Math.log10(maxBps + 1)) * 100.0;
}

function formatDataRateBps(bps)
{
    if (!Number.isFinite(bps))
    {
        return "--";
    }

    return `${Math.round(bps)} bps`;
}

function renderDataRate(rfRxBps, updatedAt)
{
    if (!el.dataRateMeter || !el.dataRateFill || !el.dataRateText)
    {
        return;
    }

    if (!Number.isFinite(rfRxBps))
    {
        el.dataRateFill.style.width = "0%";
        el.dataRateText.textContent = "-- bps";
        el.dataRateMeter.title = tr("radio.data_rate_no_value", "RX data rate: kein Wert");

        return;
    }

    const safeBps = Math.max(0, rfRxBps);
    const percent = dataRatePercent(safeBps);
    const text = formatDataRateBps(safeBps);

    el.dataRateFill.style.width = `${percent}%`;
    el.dataRateText.textContent = text;
    el.dataRateMeter.title =
        `${tr("radio.data_rate", "RX data rate")}: ${text}` +
        `\n${tr("radio.data_rate_scale", "Balken: logarithmisch skaliert bis 62500 bps")}` +
        (updatedAt ? `\n${tr("common.update", "Update")}: ${updatedAt}` : "");
}

function renderCompanionLinkStatus(connected, updatedAt)
{
    if (!el.companionLinkLed)
    {
        return;
    }

    if (connected)
    {
        el.companionLinkLed.classList.remove("offline");
        el.companionLinkLed.classList.add("online");
        el.companionLinkLed.title =
            tr("radio.companion_connected", "Companion: verbunden") +
            (updatedAt ? `\n${tr("common.update", "Update")}: ${updatedAt}` : "");
    }
    else
    {
        el.companionLinkLed.classList.remove("online");
        el.companionLinkLed.classList.add("offline");
        el.companionLinkLed.title =
            tr("radio.companion_disconnected", "Companion: nicht verbunden") +
            (updatedAt ? `\n${tr("common.last_update", "Letztes Update")}: ${updatedAt}` : "");
    }
}

function setRightPanelHeader(title, subtitle = "")
{
    if (el.rightPanelTitle)
    {
        el.rightPanelTitle.textContent = title;
    }

    if (el.rightPanelSubtitle)
    {
        el.rightPanelSubtitle.textContent = subtitle;
        el.rightPanelSubtitle.style.display = subtitle !== "" ? "" : "none";
    }
}

function updateMapActionButtons()
{
    const row = state.mapContextRow;
    const isMapMode = state.rightPanelMode === "map" && !!row;
    const publicKeyHex = String(row?.public_key_hex || "").trim();
    const canReset = /^[0-9A-Fa-f]{64}$/.test(publicKeyHex);
    const hasAdvertPath = String(row?.last_advert_path_text || "").trim() !== "";

    if (el.mapPositionButton)
    {
        el.mapPositionButton.style.display = isMapMode && hasLocation(row) ? "" : "none";
    }

    if (el.mapPathButton)
    {
        el.mapPathButton.style.display = isMapMode && hasAdvertPath ? "" : "none";
    }

    if (el.resetPathButton)
    {
        el.resetPathButton.style.display = isMapMode && canReset ? "" : "none";
        el.resetPathButton.disabled = !canReset || state.resetPathPending;
    }
}

function setRightPanelMode(mode, row = null)
{
    state.rightPanelMode = mode;
    state.mapContextRow = mode === "map" ? row : null;

    if (mode === "monitor")
    {
        setRightPanelHeader("Monitor", "meshcore_monitor");
    }
    else if (mode === "map")
    {
        let subtitle = "";

        if (row)
        {
            const rowName = String(row.name || "").trim();
            const subtitleParts = [];

            if (rowName !== "")
            {
                subtitleParts.push(rowName);
            }

            if (row.last_advert_path_len != null)
            {
                subtitleParts.push("Hops: " + row.last_advert_path_len);
            }

            if (row.last_advert_path_hash_size != null)
            {
                subtitleParts.push("Hash: " + row.last_advert_path_hash_size + " Bytes");
            }

            if (row.last_advert_path_text != null && String(row.last_advert_path_text).trim() !== "")
            {
                subtitleParts.push("Path: " + row.last_advert_path_text);
            }

            subtitle = subtitleParts.join(" / ");
        }

        setRightPanelHeader(tr("panel.map", "Map"), subtitle);
    }
    else
    {
        setRightPanelHeader(tr("panel.messages", "Messages"), "");
    }

    updateMapActionButtons();
}

function ensureChatPathMap()
{
    if (!el.chatPathMap)
    {
        return null;
    }

    if (!state.chatPathLeafletMap)
    {
        state.chatPathLeafletMap = L.map("chatPathMap",
        {
            zoomControl: true,
            wheelPxPerZoomLevel: 240,
            zoomSnap: 0.25
        });

        L.tileLayer("https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png",
        {
            maxZoom: 19,
            attribution: "&copy; OpenStreetMap"
        }).addTo(state.chatPathLeafletMap);

        state.chatPathLayer = L.layerGroup().addTo(state.chatPathLeafletMap);
    }

    return state.chatPathLeafletMap;
}

function hideChatPathMap()
{
    if (el.chatPathMapInfo)
    {
        el.chatPathMapInfo.textContent = "";
    }

    if (state.chatPathLayer)
    {
        state.chatPathLayer.clearLayers();
    }

    state.chatPathLastCorrelationKey = "";
    state.chatPathLastPreferredPath = null;

    if (state.rightTab === "pathmap")
    {
        activateChatTab("messages");
    }
}

function getPathPointDisplayName(point, showHash = state.chatPathShowHash)
{
    const name = String(point?.name || "").trim();
    const prefix = String(point?.prefix6_hex || "").trim().substring(0, 6);

    if (showHash && prefix !== "")
    {
        return `${name || prefix} [${prefix}]`;
    }

    return name || prefix || "-";
}

function getPathPointTooltipText(point)
{
    const name = String(point?.name || "").trim();
    const prefix = String(point?.prefix6_hex || "").trim().substring(0, 6);

    if (state.chatPathShowNames && state.chatPathShowHash && prefix !== "")
    {
        return `${name || prefix} [${prefix}]`;
    }

    if (state.chatPathShowNames)
    {
        return name || prefix || "-";
    }

    if (state.chatPathShowHash && prefix !== "")
    {
        return prefix;
    }

    return "";
}

function buildPreferredPathMapPoints(preferredPath)
{
    if (!preferredPath)
    {
        return [];
    }

    const points = [];

    if (
        preferredPath.source &&
        hasValidCoords(preferredPath.source)
    )
    {
        points.push(
        {
            type: "source",
            name: preferredPath.source.name || "Node",
            prefix6_hex: preferredPath.source.prefix6_hex || "",
            lat: Number(preferredPath.source.adv_lat_e6) / 1000000.0,
            lon: Number(preferredPath.source.adv_lon_e6) / 1000000.0,
            distance_m: null
        });
    }

    const hops = Array.isArray(preferredPath.hops) ? preferredPath.hops : [];

    hops.forEach(function(hop)
    {
        if (
            hop &&
            hop.resolved &&
            hop.node &&
            hasValidCoords(hop.node)
        )
        {
            points.push(
            {
                type: "hop",
                hop_index: Number(hop.hop_index),
                name: hop.node.name || hop.node.prefix6_hex || `Hop ${hop.hop_index}`,
                prefix6_hex: hop.node.prefix6_hex || "",
                lat: Number(hop.node.adv_lat_e6) / 1000000.0,
                lon: Number(hop.node.adv_lon_e6) / 1000000.0,
                token: hop.token || "",
                distance_m: Number.isFinite(Number(hop.distance_m))
                    ? Number(hop.distance_m)
                    : null
            });
        }
    });

    if (
        preferredPath.endpoint &&
        hasValidEndpointCoords(preferredPath.endpoint)
    )
    {
        points.push(
        {
            type: "endpoint",
            name: preferredPath.endpoint.name || "Endpoint",
            lat: Number(preferredPath.endpoint.latitude_e6) / 1000000.0,
            lon: Number(preferredPath.endpoint.longitude_e6) / 1000000.0,
            distance_m: null
        });
    }

    return points;
}


function getPathSegmentDistanceMeters(fromPoint, toPoint)
{
    const rawExplicitDistance = fromPoint ? fromPoint.distance_m : null;

    if (rawExplicitDistance !== null && rawExplicitDistance !== undefined && rawExplicitDistance !== "")
    {
        const explicitDistance = Number(rawExplicitDistance);

        if (Number.isFinite(explicitDistance))
        {
            return explicitDistance;
        }
    }

    if (!fromPoint || !toPoint)
    {
        return null;
    }

    const calculatedDistance = distanceMeters(
        Math.round(Number(fromPoint.lat) * 1000000.0),
        Math.round(Number(fromPoint.lon) * 1000000.0),
        Math.round(Number(toPoint.lat) * 1000000.0),
        Math.round(Number(toPoint.lon) * 1000000.0)
    );

    return Number.isFinite(calculatedDistance) ? calculatedDistance : null;
}

function showPreferredPathInPathMap(correlationKey, preferredPath)
{
    if (!state.chatRow || !isChannelRow(state.chatRow))
    {
        hideChatPathMap();
        return;
    }

    if (!el.chatTabsView || !el.chatPathMapPanel || !el.chatPathMap)
    {
        return;
    }

    const points = buildPreferredPathMapPoints(preferredPath);

    state.chatPathLastCorrelationKey = correlationKey;
    state.chatPathLastPreferredPath = preferredPath;

    if (points.length === 0)
    {
        hideChatPathMap();
        return;
    }

    const map = ensureChatPathMap();

    if (!map || !state.chatPathLayer)
    {
        return;
    }

    if (el.chatPathToggleNames)
    {
        el.chatPathToggleNames.checked = state.chatPathShowNames;
    }

    if (el.chatPathToggleHash)
    {
        el.chatPathToggleHash.checked = state.chatPathShowHash;
    }

    if (el.chatPathToggleDistances)
    {
        el.chatPathToggleDistances.checked = state.chatPathShowDistances;
    }

    state.chatPathLayer.clearLayers();

    const latlngs = [];

    points.forEach(function(point)
    {
        const latlng = [point.lat, point.lon];
        latlngs.push(latlng);

        const displayName = getPathPointDisplayName(point);
        const label = point.type === "endpoint"
            ? `Endpoint: ${displayName}`
            : `Hop ${point.hop_index}: ${displayName}`;

        const circle = L.circleMarker(latlng,
        {
            radius: 6,
            color: "#0a203bff",
            weight: 2,
            fillColor: "#60a5fa",
            fillOpacity: 0.8
        }).addTo(state.chatPathLayer);

        circle.bindPopup(label);

        const tooltipText = getPathPointTooltipText(point);

        if (tooltipText !== "")
        {
            circle.bindTooltip(escapeHtml(tooltipText),
            {
                permanent: true,
                direction: "center",
                offset: [0, -1],
                className: "path-name-label"
            });
        }
    });

    if (latlngs.length >= 2)
    {
        for (let index = 0; index < points.length - 1; index += 1)
        {
            const fromPoint = points[index];
            const toPoint = points[index + 1];
            const segmentLatLngs =
            [
                [fromPoint.lat, fromPoint.lon],
                [toPoint.lat, toPoint.lon]
            ];

            L.polyline(segmentLatLngs,
            {
                weight: 2
            }).addTo(state.chatPathLayer);

            const segmentDistanceM = getPathSegmentDistanceMeters(fromPoint, toPoint);

            if (state.chatPathShowDistances && segmentDistanceM !== null)
            {
                const midLat = (fromPoint.lat + toPoint.lat) / 2.0;
                const midLon = (fromPoint.lon + toPoint.lon) / 2.0;
                const distanceKm = segmentDistanceM / 1000.0;

                L.marker([midLat, midLon],
                {
                    interactive: false,
                    icon: L.divIcon(
                    {
                        className: "path-distance-label",
                        html: `<div>${distanceKm.toFixed(1)} km</div>`
                    })
                }).addTo(state.chatPathLayer);
            }
        }
    }

    if (el.chatPathMapInfo)
    {
        const pathText = preferredPath && preferredPath.path_text
            ? preferredPath.path_text
            : "-";

        el.chatPathMapInfo.textContent = `Key: ${correlationKey} | Path: ${pathText}`;
    }

    if (el.chatTabsView)
    {
        el.chatTabsView.style.display = "flex";
    }

    state.rightView = "pathmap";
    activateChatTab("pathmap");

    setTimeout(function()
    {
        map.invalidateSize();

        if (latlngs.length === 1)
        {
            map.setView(latlngs[0], 13);
        }
        else
        {
            map.fitBounds(latlngs, { padding: [20, 20] });
        }
    }, 0);
}

const resolvedPathsByCorrelationKey = {};

const CHAT_SYMBOL_GROUPS = [
    {
        titleKey: "symbol_group.smilies", title: "Smilies",
        symbols: ["😀", "😁", "😂", "🤣", "😊", "😉", "😍", "😘", "😎", "🤔", "😮", "😢", "😡", "👍", "👎", "👏", "🙏", "💪"]
    },
    {
        titleKey: "symbol_group.hints", title: "Hinweise",
        symbols: ["✅", "❌", "⚠️", "ℹ️", "❗", "❓", "⭐", "🔥", "💡", "📌", "📍", "🔔", "🔒", "🔓", "📡", "🔋", "🛰️", "📻"]
    },
    {
        titleKey: "symbol_group.arrows", title: "Pfeile",
        symbols: ["←", "→", "↑", "↓", "↔", "↕", "↖", "↗", "↘", "↙", "⇒", "⇐", "⇑", "⇓", "➜", "➤", "⤴", "⤵"]
    },
    {
        titleKey: "symbol_group.tech_radio", title: "Technik / Funk",
        symbols: ["⚡", "⏻", "⌁", "⎓", "⏚", "Ω", "µ", "°", "±", "×", "÷", "≈", "≠", "≤", "≥", "∞", "λ", "π"]
    },
    {
        titleKey: "symbol_group.weather_outdoor", title: "Wetter / Outdoor",
        symbols: ["☀️", "🌤️", "⛅", "☁️", "🌧️", "⛈️", "❄️", "🌫️", "🌈", "🌙", "🌍", "🌲", "🏔️", "🚁", "🧭", "⌚", "⏱️", "📶"]
    },
    {
        titleKey: "symbol_group.text_symbols", title: "Textzeichen",
        symbols: ["©", "®", "™", "§", "¶", "•", "…", "–", "—", "„", "“", "”", "‘", "’", "«", "»", "✓", "✕"]
    }
];

function buildChatSymbolPalette()
{
    if (!el.chatSymbolPalette)
    {
        return;
    }

    el.chatSymbolPalette.innerHTML = CHAT_SYMBOL_GROUPS.map(function(group)
    {
        const buttons = group.symbols.map(function(symbol)
        {
            return `<button class="chat-symbol-item" type="button" data-symbol="${escapeHtml(symbol)}" title="${escapeHtml(symbol)}">${escapeHtml(symbol)}</button>`;
        }).join("");

        return `
            <div class="chat-symbol-group">
                <div class="chat-symbol-title">${escapeHtml(tr(group.titleKey, group.title))}</div>
                <div class="chat-symbol-grid">${buttons}</div>
            </div>
        `;
    }).join("");
}

function setChatSymbolPaletteVisible(visible)
{
    if (!el.chatSymbolPalette || !el.chatSymbolButton)
    {
        return;
    }

    el.chatSymbolPalette.classList.toggle("visible", visible);
    el.chatSymbolPalette.setAttribute("aria-hidden", visible ? "false" : "true");
    el.chatSymbolButton.setAttribute("aria-expanded", visible ? "true" : "false");
}

function toggleChatSymbolPalette()
{
    if (!el.chatSymbolPalette)
    {
        return;
    }

    setChatSymbolPaletteVisible(!el.chatSymbolPalette.classList.contains("visible"));
}

function getChatInputText()
{
    if (!el.chatInput)
    {
        return "";
    }

    return (el.chatInput.textContent || "").replace(/\u00a0/g, " ");
}

function placeCaretAtEnd(element)
{
    if (!element)
    {
        return;
    }

    const selection = window.getSelection();

    if (!selection)
    {
        return;
    }

    const range = document.createRange();
    range.selectNodeContents(element);
    range.collapse(false);
    selection.removeAllRanges();
    selection.addRange(range);
}

function getChatInputCaretOffset()
{
    if (!el.chatInput)
    {
        return null;
    }

    const selection = window.getSelection();

    if (!isSelectionInsideChatInput(selection))
    {
        return null;
    }

    const range = selection.getRangeAt(0).cloneRange();
    const preCaretRange = range.cloneRange();
    preCaretRange.selectNodeContents(el.chatInput);
    preCaretRange.setEnd(range.endContainer, range.endOffset);

    return preCaretRange.toString().length;
}

function setChatInputCaretOffset(offset)
{
    if (!el.chatInput || offset === null)
    {
        return;
    }

    const targetOffset = Math.max(0, Number(offset) || 0);
    const selection = window.getSelection();

    if (!selection)
    {
        return;
    }

    const range = document.createRange();
    let currentOffset = 0;
    let found = false;

    function walk(node)
    {
        if (found)
        {
            return;
        }

        if (node.nodeType === Node.TEXT_NODE)
        {
            const nextOffset = currentOffset + node.nodeValue.length;

            if (targetOffset <= nextOffset)
            {
                range.setStart(node, targetOffset - currentOffset);
                range.collapse(true);
                found = true;
                return;
            }

            currentOffset = nextOffset;
            return;
        }

        for (const child of node.childNodes)
        {
            walk(child);

            if (found)
            {
                return;
            }
        }
    }

    walk(el.chatInput);

    if (!found)
    {
        range.selectNodeContents(el.chatInput);
        range.collapse(false);
    }

    selection.removeAllRanges();
    selection.addRange(range);
}
function isSelectionInsideChatInput(selection)
{
    if (!selection || !selection.rangeCount || !el.chatInput)
    {
        return false;
    }

    const anchorNode = selection.anchorNode;

    return anchorNode === el.chatInput || el.chatInput.contains(anchorNode);
}

function insertTextIntoChatInput(text)
{
    if (!el.chatInput || text === "")
    {
        return;
    }

    el.chatInput.focus();

    const selection = window.getSelection();
    let range = null;

    if (isSelectionInsideChatInput(selection))
    {
        range = selection.getRangeAt(0);
    }
    else
    {
        range = document.createRange();
        range.selectNodeContents(el.chatInput);
        range.collapse(false);
    }

    range.deleteContents();

    const textNode = document.createTextNode(text);
    range.insertNode(textNode);
    range.setStartAfter(textNode);
    range.setEndAfter(textNode);

    if (selection)
    {
        selection.removeAllRanges();
        selection.addRange(range);
    }

    updateChatInputHighlight();
}


function updateChatInputHighlight()
{
    if (!el.chatInput)
    {
        return;
    }

    const caretOffset = getChatInputCaretOffset();
    const maxLength = Number(el.chatInput.dataset.maxLength || 400);
    const rawText = limitTextCharacters(getChatInputText(), maxLength);
    const highlightLimit = Number(el.chatInput.dataset.highlightLimit || 60);
    const rawChars = getTextCharacters(rawText);
    const withinLimit = rawChars.slice(0, highlightLimit).join("");
    const overLimit = rawChars.slice(highlightLimit).join("");
    const nextCaretOffset = caretOffset === null ? null : Math.min(caretOffset, rawText.length);

    if (rawText === "")
    {
        el.chatInput.innerHTML = "";
        setChatInputCaretOffset(nextCaretOffset);
        return;
    }

    let html = `<span class="chat-input-normal">${escapeHtml(withinLimit)}</span>`;

    if (overLimit !== "")
    {
        html += `<span class="chat-input-overlimit">${escapeHtml(overLimit)}</span>`;
    }

    el.chatInput.innerHTML = html;
    setChatInputCaretOffset(nextCaretOffset);
}

function clearChatInput()
{
    if (!el.chatInput)
    {
        return;
    }

    el.chatInput.innerHTML = "";
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
    /*if (el.chatInput)
    {
        el.chatInput.disabled = !enabled;
    }*/
    if (el.chatInput)
    {
        el.chatInput.contentEditable = enabled ? "true" : "false";
        el.chatInput.setAttribute("aria-disabled", enabled ? "false" : "true");
    }

    if (el.chatSendButton)
    {
        el.chatSendButton.disabled = !enabled;
    }

    if (el.chatSymbolButton)
    {
        el.chatSymbolButton.disabled = !enabled;
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

function activateChatTab(tabName)
{
    const canUsePathMap = state.chatRow && isChannelRow(state.chatRow);

    state.rightTab =
        tabName === "pathmap" && canUsePathMap
            ? "pathmap"
            : "messages";

    if (el.messagesTabView)
    {
        el.messagesTabView.classList.toggle("active", state.rightTab === "messages");
    }

    if (el.pathMapTabView)
    {
        el.pathMapTabView.classList.toggle("active", state.rightTab === "pathmap");
    }

    if (el.tabMessagesViewBtn)
    {
        el.tabMessagesViewBtn.classList.toggle("active", state.rightTab === "messages");
    }

    if (el.tabPathMapViewBtn)
    {
        el.tabPathMapViewBtn.style.display = canUsePathMap ? "" : "none";
        el.tabPathMapViewBtn.classList.toggle("active", state.rightTab === "pathmap");
    }

    if (state.rightTab === "pathmap" && state.chatPathLeafletMap)
    {
        setTimeout(function()
        {
            state.chatPathLeafletMap.invalidateSize();
        }, 0);
    }
}

function hideRightPanelViews()
{
    stopMonitorRefresh();
    if (el.mapViewWrapper)
    {
        el.mapViewWrapper.style.display = "none";
    }

    if (el.chatTabsView)
    {
        el.chatTabsView.style.display = "none";
    }

    if (el.monitorView)
    {
        el.monitorView.style.display = "none";
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

async function loadCompanionRadioStatus()
{
    return await MeshCoreApi.loadCompanionRadioStatus();
}

const NOISE_FLOOR_CONFIG =
{
    minDbm: -120,
    maxDbm: -70,

    greenMaxDbm: -110,
    yellowMaxDbm: -96
};

function noiseFloorPercent(noiseFloor)
{
    const minDbm = NOISE_FLOOR_CONFIG.minDbm;
    const maxDbm = NOISE_FLOOR_CONFIG.maxDbm;
    const clamped = Math.max(minDbm, Math.min(maxDbm, noiseFloor));

    return ((clamped - minDbm) / (maxDbm - minDbm)) * 100.0;
}

function noiseFloorState(noiseFloor)
{
    if (noiseFloor <= NOISE_FLOOR_CONFIG.greenMaxDbm)
    {
        return "green";
    }

    if (noiseFloor <= NOISE_FLOOR_CONFIG.yellowMaxDbm)
    {
        return "yellow";
    }

    return "red";
}

function renderNoiseFloor(noiseFloor, updatedAt)
{
    if (!el.noiseFloorMeter || !el.noiseFloorFill || !el.noiseFloorText)
    {
        return;
    }

    if (!Number.isFinite(noiseFloor))
    {
        el.noiseFloorMeter.classList.remove("has-value");
        el.noiseFloorFill.style.width = "0%";
        el.noiseFloorFill.className = "noise-floor-fill";
        el.noiseFloorText.textContent = "--dBm";
        el.noiseFloorMeter.title = "Noise floor: kein Wert";

        return;
    }

    const roundedNoiseFloor = Math.round(noiseFloor);
    const stateLabel = noiseFloorState(noiseFloor);
    const percent = noiseFloorPercent(noiseFloor);

    el.noiseFloorMeter.classList.add("has-value");
    el.noiseFloorMeter.dataset.state = stateLabel;

    el.noiseFloorFill.className = `noise-floor-fill ${stateLabel}`;
    el.noiseFloorFill.style.width = `${percent}%`;

    el.noiseFloorText.textContent = `${roundedNoiseFloor}dBm`;
    el.noiseFloorMeter.title =
        `Noise floor: ${noiseFloor.toFixed(1)} dBm` +
        (updatedAt ? `\nUpdate: ${updatedAt}` : "");
}

async function refreshNoiseFloor()
{
    try
    {
        const data = await loadCompanionRadioStatus();
        const status = data && data.status ? data.status : null;

        const noiseFloor = status && status.noise_floor !== null ? Number(status.noise_floor) : NaN;
        const batteryMv = status && status.battery_mv !== null ? Number(status.battery_mv) : NaN;
        const rfRxBps = status && status.rf_rx_bps !== null ? Number(status.rf_rx_bps) : NaN;

        renderDataRate(rfRxBps, status ? status.updated_at : null);
        renderNoiseFloor(noiseFloor, status ? status.updated_at : null);
        renderBatteryVoltage(batteryMv, status ? status.updated_at : null);
        renderCompanionLinkStatus(status ? !!status.connected : false, status ? status.updated_at : null);
    }
    catch (err)
    {
        console.error("Noise floor refresh failed:", err);
        renderDataRate(NaN, null);
        renderNoiseFloor(NaN, null);
        renderBatteryVoltage(NaN, null);
        renderCompanionLinkStatus(false, null);
    }
}

async function startDiscover()
{
    return await MeshCoreApi.startDiscover();
}

async function loadDiscoverStatus()
{
    return await MeshCoreApi.loadDiscoverStatus();
}

async function clearDiscoverRequest()
{
    return await MeshCoreApi.clearDiscoverRequest();
}

async function startProtectedRepeaterRequest()
{
    return await MeshCoreApi.startProtectedRepeaterRequest();
}

async function loadProtectedRepeaterStatus()
{
    return await MeshCoreApi.loadProtectedRepeaterStatus();
}

function setProtectedRepeaterStatus(text)
{
    if (el.protectedRepeaterStatusText)
    {
        el.protectedRepeaterStatusText.textContent = text || "-";
    }
}

function signedInt8(value)
{
    return value > 127 ? value - 256 : value;
}

function readLe16(bytes, offset)
{
    return bytes[offset] | (bytes[offset + 1] << 8);
}

function readLe32(bytes, offset)
{
    return (
        (bytes[offset])
        | (bytes[offset + 1] << 8)
        | (bytes[offset + 2] << 16)
        | (bytes[offset + 3] << 24)
    ) >>> 0;
}

function decodeNeighboursHex(rawHex)
{
    const cleanHex = String(rawHex || "").replace(/\s+/g, "").toLowerCase();

    if (!cleanHex)
    {
        throw new Error(tr("protected_repeater.decode.no_raw_hex", "Kein raw_hex in der Antwort gefunden."));
    }

    if (cleanHex.length % 2 !== 0 || !/^[0-9a-f]+$/.test(cleanHex))
    {
        throw new Error(tr("protected_repeater.decode.invalid_hex", "raw_hex ist kein gültiger Hexstring."));
    }

    const bytes = [];

    for (let i = 0; i < cleanHex.length; i += 2)
    {
        bytes.push(parseInt(cleanHex.slice(i, i + 2), 16));
    }

    if (bytes.length < 4)
    {
        throw new Error(tr("protected_repeater.decode.too_short", "raw_hex ist zu kurz für den Header."));
    }

    const totalCount = readLe16(bytes, 0);
    const resultCount = readLe16(bytes, 2);
    const recordSize = 9;
    const neededLength = 4 + (resultCount * recordSize);

    if (bytes.length < neededLength)
    {
        throw new Error(tr("protected_repeater.decode.truncated", "raw_hex ist kürzer als result_count erwarten lässt."));
    }

    const neighbours = [];
    let pos = 4;

    for (let i = 0; i < resultCount; i++)
    {
        const pubkey = cleanHex.slice(pos * 2, (pos + 4) * 2);
        pos += 4;

        const secsAgo = readLe32(bytes, pos);
        pos += 4;

        const snrRaw = signedInt8(bytes[pos]);
        pos += 1;

        neighbours.push({
            pubkey: pubkey,
            secsAgo: secsAgo,
            snr: snrRaw / 4.0
        });
    }

    return {
        totalCount: totalCount,
        resultCount: resultCount,
        neighbours: neighbours
    };
}

function formatSnr(value)
{
    return Number.isInteger(value) ? value.toFixed(1) : String(value);
}

function formatTime(totalSeconds)
{
    const seconds = Math.max(0, Number(totalSeconds) || 0);

    const hours =
        Math.floor(seconds / 3600);

    const minutes =
        Math.floor((seconds % 3600) / 60);

    const secs =
        seconds % 60;

    return (
        String(hours).padStart(2, "0") + ":" +
        String(minutes).padStart(2, "0") + ":" +
        String(secs).padStart(2, "0")
    );
}

function formatProtectedRepeaterResult(resultJson, repeaterNames = {})
{
    if (!resultJson)
    {
        return "";
    }

    let payload = null;

    try
    {
        payload = JSON.parse(resultJson);
    }
    catch (error)
    {
        return resultJson;
    }

    try
    {
        const decoded = decodeNeighboursHex(payload.raw_hex || "");
        const lines = [
            `total_count: ${decoded.totalCount}`,
            `result_count: ${decoded.resultCount}`,
            "",
            " # pubkey   Name                 hh:mm:ss  SNR[dB]"
        ];

        decoded.neighbours.forEach(function(neighbour, index)
        {
            const repeaterName =
                repeaterNames[neighbour.pubkey] || "-";

            lines.push(
                `${String(index + 1).padStart(2, " ")} ` +
                `${neighbour.pubkey} ` +
                `${repeaterName.padEnd(20, " ")} ` +
                `${formatTime(neighbour.secsAgo).padStart(8, " ")} ` +
                `${formatSnr(neighbour.snr).padStart(7, " ")}`
            );
        });

        return lines.join("\n");
    }
    catch (error)
    {
        return `${tr("protected_repeater.decode.failed", "Antwort konnte nicht dekodiert werden:")} ${error.message}\n\n${resultJson}`;
    }
}

function renderProtectedRepeaterStatus(data)
{
    const action = data && data.action ? data.action : null;
    const config = data && data.config ? data.config : null;
    const targetName = action && action.target_name
        ? action.target_name
        : (config && config.protected_repeater_name ? config.protected_repeater_name : "");

    if (el.protectedRepeaterNameInput && document.activeElement !== el.protectedRepeaterNameInput)
    {
        el.protectedRepeaterNameInput.value = targetName || "";
    }

    if (el.protectedRepeaterResult)
    {
        el.protectedRepeaterResult.textContent = action && action.result_json
            ? formatProtectedRepeaterResult(action.result_json, action.repeater_names || {})
            : "";
    }

    if (!action)
    {
        setProtectedRepeaterStatus(tr("protected_repeater.status.ready", "Bereit"));

        if (el.protectedRepeaterStartButton)
        {
            el.protectedRepeaterStartButton.disabled = false;
        }

        return;
    }

    if (Number(action.status) === 3)
    {
        state.protectedRepeaterPending = false;
        setProtectedRepeaterStatus(tr("protected_repeater.status.done", "Antwort verfügbar"));

        if (el.protectedRepeaterStartButton)
        {
            el.protectedRepeaterStartButton.disabled = false;
        }

        return;
    }

    if (Number(action.status) === 2)
    {
        state.protectedRepeaterPending = false;
        setProtectedRepeaterStatus(tr("protected_repeater.status.failed", "Fehler"));

        if (el.protectedRepeaterStartButton)
        {
            el.protectedRepeaterStartButton.disabled = false;
        }

        return;
    }

    setProtectedRepeaterStatus(tr("protected_repeater.status.waiting", "Warte auf Antwort ..."));

    if (el.protectedRepeaterStartButton)
    {
        el.protectedRepeaterStartButton.disabled = true;
    }
}

async function refreshProtectedRepeaterModal()
{
    try
    {
        const data = await loadProtectedRepeaterStatus();

        if (el.protectedRepeaterModalError)
        {
            el.protectedRepeaterModalError.textContent = "";
            el.protectedRepeaterModalError.style.display = "none";
        }

        renderProtectedRepeaterStatus(data);
    }
    catch (error)
    {
        if (el.protectedRepeaterModalError)
        {
            el.protectedRepeaterModalError.textContent =
                error.message || tr("protected_repeater.error.status_failed", "Status konnte nicht geladen werden.");
            el.protectedRepeaterModalError.style.display = "block";
        }
    }
}

async function saveProtectedRepeaterNameBeforeRequest()
{
    const homeRepeaterName = el.protectedRepeaterNameInput ? el.protectedRepeaterNameInput.value.trim() : "";

    if (homeRepeaterName.length > 64)
    {
        throw new Error(tr("setup.error.home_repeater_too_long", "Home Repeater ist zu lang."));
    }

    const setupData = await loadCompanionSetup();
    const cfg = setupData && setupData.config ? setupData.config : null;

    if (!cfg)
    {
        throw new Error(tr("setup.error.load_failed", "Setup-Werte konnten nicht geladen werden."));
    }

    const name = cfg.name ? String(cfg.name).trim() : "";
    const latitude = Number(cfg.latitude);
    const longitude = Number(cfg.longitude);

    if (name === "" || !Number.isFinite(latitude) || !Number.isFinite(longitude))
    {
        throw new Error(tr("setup.error.load_failed", "Setup-Werte konnten nicht geladen werden."));
    }

    await MeshCoreApi.applyCompanionSetup(
    {
        name: name,
        location_name: cfg.location_name || "",
        protected_repeater_name: homeRepeaterName,
        latitude: latitude,
        longitude: longitude,
        bot: cfg.bot === true || Number(cfg.bot) === 1
    });
}

async function handleProtectedRepeaterStartClick()
{
    if (state.protectedRepeaterPending)
    {
        return;
    }

    state.protectedRepeaterPending = true;
    state.protectedRepeaterActionId = null;

    if (el.protectedRepeaterModalError)
    {
        el.protectedRepeaterModalError.textContent = "";
        el.protectedRepeaterModalError.style.display = "none";
    }

    if (el.protectedRepeaterResult)
    {
        el.protectedRepeaterResult.textContent = "";
    }

    setProtectedRepeaterStatus(tr("protected_repeater.status.waiting", "Warte auf Antwort ..."));

    if (el.protectedRepeaterStartButton)
    {
        el.protectedRepeaterStartButton.disabled = true;
    }

    try
    {
        await saveProtectedRepeaterNameBeforeRequest();
        const response = await startProtectedRepeaterRequest();
        state.protectedRepeaterActionId = response && response.action_id ? Number(response.action_id) : null;
        await refreshProtectedRepeaterModal();
    }
    catch (error)
    {
        state.protectedRepeaterPending = false;

        if (el.protectedRepeaterStartButton)
        {
            el.protectedRepeaterStartButton.disabled = false;
        }

        if (el.protectedRepeaterModalError)
        {
            el.protectedRepeaterModalError.textContent =
                error.message || tr("protected_repeater.error.start_failed", "Abfrage konnte nicht gestartet werden.");
            el.protectedRepeaterModalError.style.display = "block";
        }
    }
}

function stopProtectedRepeaterPolling()
{
    if (state.protectedRepeaterPollTimer)
    {
        clearInterval(state.protectedRepeaterPollTimer);
        state.protectedRepeaterPollTimer = null;
    }
}

function startProtectedRepeaterPolling()
{
    stopProtectedRepeaterPolling();

    state.protectedRepeaterPollTimer = setInterval(function()
    {
        if (!state.protectedRepeaterModalOpen)
        {
            return;
        }

        refreshProtectedRepeaterModal();
    }, 1000);
}

async function openProtectedRepeaterDialog()
{
    if (!el.protectedRepeaterModal)
    {
        return;
    }

    state.protectedRepeaterModalOpen = true;

    if (el.protectedRepeaterModalError)
    {
        el.protectedRepeaterModalError.textContent = "";
        el.protectedRepeaterModalError.style.display = "none";
    }

    el.protectedRepeaterModal.classList.add("visible");
    el.protectedRepeaterModal.setAttribute("aria-hidden", "false");

    await refreshProtectedRepeaterModal();

    if (el.protectedRepeaterNameInput)
    {
        el.protectedRepeaterNameInput.focus();
    }

    startProtectedRepeaterPolling();
}

function closeProtectedRepeaterDialog()
{
    state.protectedRepeaterModalOpen = false;
    stopProtectedRepeaterPolling();

    if (!el.protectedRepeaterModal)
    {
        return;
    }

    el.protectedRepeaterModal.classList.remove("visible");
    el.protectedRepeaterModal.setAttribute("aria-hidden", "true");

    if (el.protectedRepeaterModalError)
    {
        el.protectedRepeaterModalError.textContent = "";
        el.protectedRepeaterModalError.style.display = "none";
    }
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


const MONITOR_COLUMNS = [
    "id",
    "created_at",
    "source",
    "push_code",
    "payload_len",
    "payload_hex",
    "packet_valid",
    "decode_error",
    "snr_db",
    "rssi_dbm",
    "route_type",
    "payload_type",
    "payload_version",
    "path_len",
    "path_hash_size",
    "path_text",
    "pkt_hash",
    "rf_packet_hex",
    "pkt_payload_len",
    "pkt_payload_hex",
    "req_valid",
    "req_dst_hash",
    "req_src_hash",
    "req_mac",
    "req_cipher_len",
    "grp_txt_valid",
    "grp_channel_hash",
    "grp_mac",
    "grp_decrypt_tried",
    "grp_decrypt_ok",
    "grp_mac_verified",
    "grp_timestamp",
    "grp_txt_type",
    "grp_channel_name",
    "grp_channel_key_hex",
    "grp_text",
    "advert_valid",
    "advert_public_key_hex",
    "advert_timestamp",
    "advert_flags",
    "advert_role",
    "advert_has_gps",
    "advert_has_ble",
    "advert_has_shortcut",
    "advert_has_name",
    "advert_latitude_e6",
    "advert_longitude_e6",
    "advert_name"
];

const MONITOR_DEFAULT_COLUMNS = [
    "payload_type",
    "path_len",
    "path_hash_size",
    "path_text",
    "advert_name"
];

const MONITOR_PAYLOAD_TYPE_NAMES =
{
    0x00: "REQ",
    0x01: "RESPONSE",
    0x02: "TXT_MSG",
    0x03: "ACK",
    0x04: "ADVERT",
    0x05: "GRP_TXT",
    0x06: "GRP_DATA",
    0x07: "ANON_REQ",
    0x08: "PATH",
    0x09: "TRACE",
    0x0A: "MULTIPART",
    0x0B: "CONTROL",
    0x0C: "RESERVED_C",
    0x0D: "RESERVED_D",
    0x0E: "RESERVED_E",
    0x0F: "RAW_CUSTOM"
};

function loadMonitorVisibleColumns()
{
    try
    {
        const parsed = JSON.parse(localStorage.getItem(MONITOR_VISIBLE_COLUMNS_STORAGE_KEY) || "null");

        if (Array.isArray(parsed))
        {
            const valid = parsed.filter(function(name)
            {
                return MONITOR_COLUMNS.includes(name);
            });

            if (valid.length > 0)
            {
                return valid;
            }
        }
    }
    catch (err)
    {
        // localStorage may be disabled/private. Use defaults.
    }

    return MONITOR_DEFAULT_COLUMNS.slice();
}

function saveMonitorVisibleColumns()
{
    try
    {
        localStorage.setItem(
            MONITOR_VISIBLE_COLUMNS_STORAGE_KEY,
            JSON.stringify(state.monitorVisibleColumns || MONITOR_DEFAULT_COLUMNS)
        );
    }
    catch (err)
    {
        // localStorage may be disabled/private. Ignore and keep current session state.
    }
}

function loadMonitorColumnWidths()
{
    try
    {
        const parsed = JSON.parse(localStorage.getItem(MONITOR_COLUMN_WIDTHS_STORAGE_KEY) || "null");

        if (parsed && typeof parsed === "object" && !Array.isArray(parsed))
        {
            const widths = {};

            Object.keys(parsed).forEach(function(field)
            {
                const width = Number(parsed[field]);

                if (MONITOR_COLUMNS.includes(field) && Number.isFinite(width) && width >= 40)
                {
                    widths[field] = Math.round(width);
                }
            });

            return widths;
        }
    }
    catch (err)
    {
        // localStorage may be disabled/private. Use default widths.
    }

    return {};
}

function saveMonitorColumnWidths()
{
    try
    {
        localStorage.setItem(
            MONITOR_COLUMN_WIDTHS_STORAGE_KEY,
            JSON.stringify(state.monitorColumnWidths || {})
        );
    }
    catch (err)
    {
        // localStorage may be disabled/private. Ignore and keep current session state.
    }
}

function rememberMonitorColumnWidth(column)
{
    if (!column || typeof column.getField !== "function" || typeof column.getWidth !== "function")
    {
        return;
    }

    const field = column.getField();
    const width = Number(column.getWidth());

    if (!MONITOR_COLUMNS.includes(field) || !Number.isFinite(width) || width < 40)
    {
        return;
    }

    if (!state.monitorColumnWidths)
    {
        state.monitorColumnWidths = loadMonitorColumnWidths();
    }

    state.monitorColumnWidths[field] = Math.round(width);
    saveMonitorColumnWidths();
}

function applyMonitorColumnWidths()
{
    if (!state.monitorTable)
    {
        return;
    }

    if (!state.monitorColumnWidths)
    {
        state.monitorColumnWidths = loadMonitorColumnWidths();
    }

    Object.keys(state.monitorColumnWidths).forEach(function(field)
    {
        const width = Number(state.monitorColumnWidths[field]);

        if (!Number.isFinite(width) || width < 40)
        {
            return;
        }

        try
        {
            const column = state.monitorTable.getColumn(field);

            if (column && typeof column.setWidth === "function")
            {
                column.setWidth(width);
            }
        }
        catch (err)
        {
            // The column may be hidden or not rendered yet. Try again on the next refresh/redraw.
        }
    });
}

function formatMonitorHexNumber(value, width)
{
    if (value === null || value === undefined || value === "")
    {
        return "";
    }

    const numberValue = Number(value);

    if (!Number.isFinite(numberValue))
    {
        return String(value);
    }

    return "0x" + (numberValue >>> 0).toString(16).toUpperCase().padStart(width, "0");
}

function formatMonitorPayloadType(value)
{
    if (value === null || value === undefined || value === "")
    {
        return "";
    }

    const numberValue = Number(value);

    if (!Number.isFinite(numberValue))
    {
        return String(value);
    }

    const name = MONITOR_PAYLOAD_TYPE_NAMES[numberValue];
    const hex = formatMonitorHexNumber(numberValue, 2);

    if (!name)
    {
        return hex + "\nUNKNOWN";
    }

    return hex + " " + name;
}

function formatMonitorRow(row)
{
    const formatted = Object.assign({}, row);

    formatted.pkt_hash = formatMonitorHexNumber(row.pkt_hash, 8);
    formatted.payload_type = formatMonitorPayloadType(row.payload_type);

    return formatted;
}

function formatMonitorRows(rows)
{
    return rows.map(formatMonitorRow);
}

function getMonitorLastId(rows)
{
    return rows.reduce(function(maxId, row)
    {
        const id = Number(row.id);

        return Number.isFinite(id) && id > maxId ? id : maxId;
    }, 0);
}

function getMonitorTableHolder()
{
    if (!el.monitorTable)
    {
        return null;
    }

    return el.monitorTable.querySelector(".tabulator-tableholder");
}

function isMonitorScrolledToTop()
{
    const tableHolder = getMonitorTableHolder();

    return !tableHolder || tableHolder.scrollTop <= 2;
}

function scrollMonitorToNewest()
{
    const tableHolder = getMonitorTableHolder();

    if (tableHolder)
    {
        tableHolder.scrollTop = 0;
        return;
    }

    if (!state.monitorTable || state.monitorRows.length === 0)
    {
        return;
    }

    const newestRow = state.monitorRows[0];

    if (!newestRow || newestRow.id === undefined || newestRow.id === null)
    {
        return;
    }

    requestAnimationFrame(function()
    {
        try
        {
            state.monitorTable.scrollToRow(newestRow.id, "top", false);
        }
        catch (err)
        {
            // The row may not exist while Tabulator is still rendering. Ignore this refresh cycle.
        }
    });
}

function buildMonitorColumnDefinition(field)
{
    const isLongText = /hex$/.test(field) || field === "payload_hex" || field === "rf_packet_hex" || field === "pkt_payload_hex";
    const savedWidth = state.monitorColumnWidths ? Number(state.monitorColumnWidths[field]) : NaN;

    return {
        title: field,
        field: field,
        headerSort: true,
        visible: state.monitorVisibleColumns.includes(field),
        width: Number.isFinite(savedWidth) && savedWidth >= 40 ? savedWidth : (isLongText ? 260 : undefined),
        minWidth: isLongText ? 180 : 90,
        formatter: "plaintext"
    };
}

function renderMonitorColumnToolbar()
{
    if (!el.monitorColumnToolbar)
    {
        return;
    }

    if (!Array.isArray(state.monitorVisibleColumns))
    {
        state.monitorVisibleColumns = loadMonitorVisibleColumns();
    }

    el.monitorColumnToolbar.innerHTML = MONITOR_COLUMNS.map(function(field)
    {
        const checked = state.monitorVisibleColumns.includes(field) ? " checked" : "";

        return "<label class=\"monitor-column-toggle\">" +
            "<input type=\"checkbox\" data-monitor-column=\"" + escapeHtml(field) + "\"" + checked + ">" +
            "<span>" + escapeHtml(field) + "</span>" +
            "</label>";
    }).join("");

    el.monitorColumnToolbar.querySelectorAll("input[data-monitor-column]").forEach(function(input)
    {
        input.addEventListener("change", function()
        {
            const field = input.getAttribute("data-monitor-column");

            if (input.checked)
            {
                if (!state.monitorVisibleColumns.includes(field))
                {
                    state.monitorVisibleColumns.push(field);
                }
            }
            else
            {
                state.monitorVisibleColumns = state.monitorVisibleColumns.filter(function(name)
                {
                    return name !== field;
                });
            }

            saveMonitorVisibleColumns();

            if (state.monitorTable)
            {
                if (input.checked)
                {
                    state.monitorTable.showColumn(field);
                    applyMonitorColumnWidths();
                }
                else
                {
                    state.monitorTable.hideColumn(field);
                }

                state.monitorTable.redraw(true);
                applyMonitorColumnWidths();
            }
        });
    });
}

function ensureMonitorTable()
{
    if (!el.monitorTable || typeof Tabulator !== "function")
    {
        return null;
    }

    if (!Array.isArray(state.monitorVisibleColumns))
    {
        state.monitorVisibleColumns = loadMonitorVisibleColumns();
    }

    if (!state.monitorColumnWidths)
    {
        state.monitorColumnWidths = loadMonitorColumnWidths();
    }

    if (!state.monitorTable)
    {
        state.monitorTable = new Tabulator("#monitorTable",
        {
            data: [],
            layout: "fitData",
            height: "100%",
            index: "id",
            movableColumns: true,
            columnDefaults:
            {
                resizable: true,
                headerFilter: "input"
            },
            columns: MONITOR_COLUMNS.map(buildMonitorColumnDefinition)
        });

        state.monitorTable.on("columnResized", rememberMonitorColumnWidth);
        state.monitorTable.on("tableBuilt", applyMonitorColumnWidths);
    }

    return state.monitorTable;
}

function stopMonitorRefresh()
{
    if (state.monitorRefreshTimer)
    {
        clearInterval(state.monitorRefreshTimer);
        state.monitorRefreshTimer = null;
    }
}

async function refreshMonitor(autoScrollOnNewData)
{
    const monitorTable = ensureMonitorTable();

    if (!monitorTable || state.monitorRefreshBusy)
    {
        return;
    }

    state.monitorRefreshBusy = true;

    try
    {
        const previousLastId = state.monitorLastId;
        const wasAtTop = isMonitorScrolledToTop();
        const tableHolder = getMonitorTableHolder();
        const previousScrollTop = tableHolder ? tableHolder.scrollTop : 0;
        const data = await MeshCoreApi.loadMonitor();
        const rows = formatMonitorRows(Array.isArray(data.rows) ? data.rows : []);
        const newestId = getMonitorLastId(rows);
        const hasNewRows = newestId > previousLastId;

        state.monitorRows = rows;
        state.monitorLastId = newestId;

        setRightPanelHeader("Monitor", rows.length + " Zeilen aus meshcore_monitor");
        await monitorTable.replaceData(rows);
        monitorTable.redraw(true);
        applyMonitorColumnWidths();

        if (autoScrollOnNewData && hasNewRows && wasAtTop)
        {
            scrollMonitorToNewest();
        }
        else if (!wasAtTop && tableHolder)
        {
            requestAnimationFrame(function()
            {
                tableHolder.scrollTop = previousScrollTop;
            });
        }
    }
    catch (error)
    {
        console.error("Monitor laden fehlgeschlagen:", error);
        setRightPanelHeader("Monitor", "Fehler beim Laden");
        monitorTable.setData([
        {
            id: "Fehler",
            decode_error: error.message || String(error)
        }]);
    }
    finally
    {
        state.monitorRefreshBusy = false;
    }
}

function startMonitorRefresh()
{
    stopMonitorRefresh();

    state.monitorRefreshTimer = setInterval(function()
    {
        refreshMonitor(true);
    }, MONITOR_REFRESH_INTERVAL_MS);
}

async function showMonitor()
{
    stopCurrentChatRefresh();
    resetChatState();
    hideChatPathMap();
    hideRightPanelViews();
    setChatInputEnabled(false);
    state.rightView = "monitor";
    setRightPanelMode("monitor", null);

    if (el.monitorView)
    {
        el.monitorView.style.display = "flex";
    }

    renderMonitorColumnToolbar();

    const monitorTable = ensureMonitorTable();

    if (!monitorTable)
    {
        return;
    }

    state.monitorLastId = 0;
    monitorTable.setData([]);

    await refreshMonitor(true);
    startMonitorRefresh();
}

function showEmptyRightPanel()
{
    stopCurrentChatRefresh();
    resetChatState();
    state.rightView = "empty";
    setRightPanelMode("messages", null);

    setChatInputEnabled(false);
    hideRightPanelViews();
    hideChatPathMap();

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
    state.rightView = "map";
    setRightPanelMode("map", row);

    setChatInputEnabled(false);
    hideRightPanelViews();
    hideChatPathMap();

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
    setRightPanelMode("map", row);
    resetChatState();
    setChatInputEnabled(false);

    const pos = getNodeLatLon(row);

    if (!pos || !el.mapView)
    {
        showInfoForRow(row);
        return;
    }

    hideRightPanelViews();
    hideChatPathMap();

    if (el.mapViewWrapper)
    {
        el.mapViewWrapper.style.display = "block";
    }

    if (el.mapPathControls)
    {
        el.mapPathControls.style.display = "none";
    }

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

    if (el.mapPathControls)
    {
        el.mapPathControls.style.display = "none";
    }
}

async function showNodeAdvertPath(row)
{
    if (!row || !row.id)
    {
        return;
    }

    state.lastNodeAdvertPathRow = row;

    const pathText = String(row.last_advert_path_text || "").trim();

    if (pathText === "")
    {
        showMapForRow(row);
        return;
    }


    try
    {
        state.rightView = "map";
        setRightPanelMode("map", row);
        resetChatState();
        setChatInputEnabled(false);

        const data = await MeshCoreApi.loadNodePath(row.id);

        const endpoint = data.endpoint || null;
        const pathEntry = data.path || null;

        if (!pathEntry)
        {
            showMapForRow(row);
            return;
        }

        const sourceNode = data.node || row || null;
        const resolvedPath = resolvePathBestRoute(pathEntry, endpoint, sourceNode);
        const preferredPath = buildResolvedPathListEntry(resolvedPath, endpoint);

        preferredPath.source =
        {
            name: data.node?.name || row.name || "",
            prefix6_hex: data.node?.prefix6_hex || row.prefix6_hex || "",
            adv_lat_e6: data.node?.adv_lat_e6 ?? row.adv_lat_e6 ?? null,
            adv_lon_e6: data.node?.adv_lon_e6 ?? row.adv_lon_e6 ?? null
        };

        hideRightPanelViews();
        hideChatPathMap();

        if (!el.mapView)
        {
            return;
        }

        el.mapViewWrapper.style.display = "block";

        if (el.mapPathControls)
        {
            el.mapPathControls.style.display = "flex";
        }

        if (el.mapPathToggleNames)
        {
            el.mapPathToggleNames.checked = state.chatPathShowNames;
        }

        if (el.mapPathToggleHash)
        {
            el.mapPathToggleHash.checked = state.chatPathShowHash;
        }

        if (el.mapPathToggleDistances)
        {
            el.mapPathToggleDistances.checked = state.chatPathShowDistances;
        }

        const map = ensureMap();

        if (!map || !state.leafletMarkers)
        {
            return;
        }

        const points = buildPreferredPathMapPoints(preferredPath);

        if (points.length === 0)
        {
            showMapForRow(row);
            return;
        }

        state.leafletMarkers.clearLayers();

        const latlngs = [];

        points.forEach(function(point)
        {
            const latlng = [point.lat, point.lon];
            latlngs.push(latlng);

            const displayName = getPathPointDisplayName(point);
            let label = displayName;

            if (point.type === "source")
            {
                label = `Node: ${displayName}`;
            }
            else if (point.type === "endpoint")
            {
                label = `Endpoint: ${displayName}`;
            }
            else
            {
                label = `Hop ${point.hop_index}: ${displayName}`;
            }

            const circle = L.circleMarker(latlng,
            {
                radius: point.type === "source" ? 8 : 6,
                color: "#0a203bff",
                weight: 2,
                fillColor: "#60a5fa",
                fillOpacity: 0.8
            }).addTo(state.leafletMarkers);

            circle.bindPopup(escapeHtml(label));

            const tooltipText = getPathPointTooltipText(point);

            if (tooltipText !== "")
            {
                circle.bindTooltip(escapeHtml(tooltipText),
                {
                    permanent: true,
                    direction: "center",
                    offset: [0, -1],
                    className: "path-name-label"
                });
            }
        });

        if (latlngs.length >= 2)
        {
            for (let index = 0; index < points.length - 1; index += 1)
            {
                const fromPoint = points[index];
                const toPoint = points[index + 1];

                L.polyline(
                [
                    [fromPoint.lat, fromPoint.lon],
                    [toPoint.lat, toPoint.lon]
                ],
                {
                    weight: 2
                }).addTo(state.leafletMarkers);

                const segmentDistanceM = getPathSegmentDistanceMeters(fromPoint, toPoint);

                if (state.chatPathShowDistances && segmentDistanceM !== null)
                {
                    const midLat = (fromPoint.lat + toPoint.lat) / 2.0;
                    const midLon = (fromPoint.lon + toPoint.lon) / 2.0;
                    const distanceKm = segmentDistanceM / 1000.0;

                    L.marker([midLat, midLon],
                    {
                        interactive: false,
                        icon: L.divIcon(
                        {
                            className: "path-distance-label",
                            html: `<div>${distanceKm.toFixed(1)} km</div>`
                        })
                    }).addTo(state.leafletMarkers);
                }
            }
        }

        setTimeout(function()
        {
            map.invalidateSize();

            if (latlngs.length === 1)
            {
                map.setView(latlngs[0], 13);
            }
            else
            {
                map.fitBounds(latlngs, { padding: [30, 30] });
            }
        }, 0);
    }
    catch (error)
    {
        console.error("Advert-Pfad konnte nicht geladen werden:", error);
        window.alert(`Advert-Pfad konnte nicht geladen werden: ${error.message || "Unbekannter Fehler"}`);
    }

    
}

function showAllNodesMap()
{
    stopCurrentChatRefresh();
    resetChatState();
    state.rightView = "allmap";
    setRightPanelMode("map", null);
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
    hideChatPathMap();
    el.mapViewWrapper.style.display = "block";

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

    if (el.mapPathControls)
    {
        el.mapPathControls.style.display = "none";
    }
}

function relativeTime(cell)
{
    const value = cell.getRow().getData().last_advert_at;

    if (!value)
    {
        return '<div class="last-seen unknown">?</div>';
    }

    const timestamp = normalizeGuiTimestamp(parseMariaDbDateTime(value));

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

        const ts = normalizeGuiTimestamp(parseMariaDbDateTime(value));

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

    const newestMessageEpoch = Number(row.newest_msg_epoch || 0);
    const lastReadEpoch = getContactLastReadEpoch(row);
    const unreadClass = newestMessageEpoch > lastReadEpoch ? " has-unread" : "";

    const badge = msgCount > 0
        ? `<span class="message-badge${unreadClass}">💬 ${msgCount}</span>`
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

function renderOutgoingMessage(msg)
{
    const dateText = escapeHtml(
        Number(msg.timestamp_epoch || 0) > 0
            ? formatEpochDateTime(msg.timestamp_epoch)
            : formatDateTime(msg.received_at)
    );

    const rawText = String(msg.text || msg.message_text || "");
    const textValue = formatMessageText(rawText);
    const statusText = escapeHtml(String(msg.ui_status_text || tr("chat.status.pending", "Pending")));
    const statusClass = getOutgoingStatusClass(msg);

    const resendButton = `
        <button type="button"
                class="chat-action-button chat-resend-button"
                data-message-id="${escapeHtml(String(msg.id || ""))}"
                title="${escapeHtml(tr("chat.resend", "Nochmal senden"))}">↻</button>
    `;

    return `
        <div class="chat-message outgoing" data-tx-id="${escapeHtml(String(msg.tx_outbox_id || ""))}">
            <div class="chat-message-header">
                <div class="chat-message-meta">
                    <span>${dateText}</span>
                    <span>${escapeHtml(tr("chat.you", "Du"))}</span>
                </div>
                <div class="chat-message-actions">${resendButton}</div>
            </div>
            <div class="chat-message-text">${textValue}</div>
            <div class="chat-message-status ${statusClass}">${statusText}</div>
        </div>
    `;
}

function insertReplyMention(name)
{
    if (!el.chatInput)
    {
        return;
    }

    const cleanName = String(name || "").trim();

    if (cleanName === "")
    {
        return;
    }

    const mention = "@" + cleanName + ": ";

    if ("value" in el.chatInput)
    {
        const currentValue = String(el.chatInput.value || "");

        if (!currentValue.startsWith(mention))
        {
            el.chatInput.value = mention + currentValue;
        }

        el.chatInput.focus();

        if (typeof el.chatInput.setSelectionRange === "function")
        {
            const pos = el.chatInput.value.length;
            el.chatInput.setSelectionRange(pos, pos);
        }

        if (typeof el.chatInput.dispatchEvent === "function")
        {
            el.chatInput.dispatchEvent(new Event("input", { bubbles: true }));
        }

        return;
    }

    if (el.chatInput.isContentEditable)
    {
        const currentText = String(el.chatInput.textContent || "");

        if (!currentText.startsWith(mention))
        {
            el.chatInput.textContent = mention + currentText;
        }

        el.chatInput.focus();

        const selection = window.getSelection();

        if (selection)
        {
            const range = document.createRange();
            range.selectNodeContents(el.chatInput);
            range.collapse(false);
            selection.removeAllRanges();
            selection.addRange(range);
        }

        if (typeof el.chatInput.dispatchEvent === "function")
        {
            el.chatInput.dispatchEvent(new Event("input", { bubbles: true }));
        }
    }
}

function renderIncomingMessage(msg)
{
    const dateText = escapeHtml(
        Number(msg.timestamp_epoch || 0) > 0
            ? formatEpochDateTime(msg.timestamp_epoch)
            : formatDateTime(msg.received_at)
    );

    const textValue = formatMessageText(msg.text || msg.message_text || "");
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

    const replyName = roomSenderName !== ""
        ? roomSenderName
        : extractReplyNameFromMessage(msg);

    const correlationKey = String(msg.correlation_key || "").trim();

    const replyButton =
        replyName !== ""
            ? `<button type="button"
                       class="chat-action-button chat-reply-button"
                       data-reply-name="${escapeHtml(replyName)}"
                       title="${escapeHtml(tr("chat.reply_to", "Antwort an"))} ${escapeHtml(replyName)}">@</button>`
            : "";

    const pathButton =
        correlationKey !== ""
            ? `<button type="button"
                       class="chat-action-button chat-path-button"
                       data-correlation-key="${escapeHtml(correlationKey)}"
                       title="${escapeHtml(tr("chat.show_path", "Pfad anzeigen"))}">⤳</button>`
            : "";

    const actionButtons =
        (replyButton !== "" || pathButton !== "")
            ? `<div class="chat-message-actions">${replyButton}${pathButton}</div>`
            : "";

    return `
        <div class="chat-message">
            <div class="chat-message-header">
                <div class="chat-message-meta">
                    <span>${dateText}</span>
                    <span>${escapeHtml(tr("chat.snr", "SNR"))}: ${escapeHtml(snr)}</span>
                    <span>${escapeHtml(tr("chat.path", "Path"))}: ${escapeHtml(path)}</span>
                </div>
                ${actionButtons}
            </div>
            ${senderBlock}
            <div class="chat-message-text">${textValue}</div>
        </div>
    `;
}
 

function debugPrintPreferredResolvedPath(preferredPath)
{
    if (!preferredPath)
    {
        console.debug("PreferredResolvedPath: keiner gefunden");
        return;
    }

    consoledebug("PreferredResolvedPath (object):", preferredPath);

    try
    {
        consoledebug(
            "PreferredResolvedPath (pretty JSON):\n" +
            JSON.stringify(preferredPath/*, null, 2*/)
        );
    }
    catch (error)
    {
        console.error("PreferredResolvedPath JSON stringify fehlgeschlagen:", error);
    }
}

function debugPrintResolvedPath(resolvedPath, endpoint)
{
    consoledebug(
        `Greedy-Aufloesung: path_id=${resolvedPath.path_id}, mode=${resolvedPath.resolution_mode}, resolved=${resolvedPath.resolved}`
    );

    if (hasValidEndpointCoords(endpoint))
    {
        consoledebug(
            `  Endpunkt: ${endpoint.name || ""} (${endpoint.latitude_e6}, ${endpoint.longitude_e6})`
        );
    }

    const hops = Array.isArray(resolvedPath.resolved_hops) ? resolvedPath.resolved_hops : [];

    for (let index = hops.length - 1; index >= 0; index -= 1)
    {
        const hop = hops[index];

        consoledebug(
            `  [Resolved Hop ${hop.hop_index}] token=${hop.token}, original_match_count=${hop.original_match_count}, note=${hop.note}`
        );

        if (!hop.selected)
        {
            consoledebug("    kein eindeutiger Kandidat bestimmbar");
            continue;
        }

        let targetLabel = "unbekannt";

        if (index === hops.length - 1)
        {
            targetLabel = `Endpunkt ${endpoint && endpoint.name ? endpoint.name : ""}`.trim();
        }
        else
        {
            const nextHop = hops[index + 1];

            if (nextHop && nextHop.selected)
            {
                targetLabel = `Hop ${nextHop.hop_index} ${nextHop.selected.name || nextHop.selected.prefix6_hex || ""}`.trim();
            }
            else if (nextHop)
            {
                targetLabel = `Hop ${nextHop.hop_index} (unaufgeloest)`;
            }
        }

        consoledebug(
            `    Segment: Hop ${hop.hop_index} -> ${targetLabel}, distance=${formatDistanceMeters(hop.selected.distance_m)}`
        );

        consoledebug(
            `    selected: prefix6_hex=${hop.selected.prefix6_hex}, name=${hop.selected.name}, adv_lat_e6=${hop.selected.adv_lat_e6}, adv_lon_e6=${hop.selected.adv_lon_e6}`
        );
    }
}

async function handleMessagePathClick(correlationKey)
{
    const key = String(correlationKey || "").trim();

    if (key === "")
    {
        consoledebug("handleMessagePathClick: leerer correlation_key");
        return;
    }

    try
    {
        const data = await MeshCoreApi.loadMessagePath(key);

        const paths = Array.isArray(data.paths) ? data.paths : [];
        const endpoint = data.endpoint || null;

        consoledebug("Path lookup:", key, "Treffer:", paths.length);

        if (endpoint)
        {
            consoledebug(
                `Endpunkt: id=${endpoint.id}, name=${endpoint.name || ""}, latitude_e6=${endpoint.latitude_e6}, longitude_e6=${endpoint.longitude_e6}`
            );
        }
        else
        {
            consoledebug("Endpunkt: nicht vorhanden");
        }

        if (paths.length === 0)
        {
            consoledebug("Keine rx_log_messages für correlation_key gefunden.");
            return;
        }

        paths.forEach(function(pathEntry, pathIndex)
        {
            consoledebug(
                `[Pfad ${pathIndex}] id=${pathEntry.id}, created_at=${pathEntry.created_at}, path_text=${pathEntry.path_text || ""}, hops=${pathEntry.hop_count || 0}`
            );

            const hops = Array.isArray(pathEntry.hops) ? pathEntry.hops : [];

            hops.forEach(function(hop, hopIndex)
            {
                consoledebug(
                    `  [Hop ${hopIndex}] token=${hop.token}, token_len=${hop.token_len}, match_count=${hop.match_count}`
                );

                const matches = Array.isArray(hop.matches) ? hop.matches : [];

                if (matches.length === 0)
                {
                    consoledebug("    keine Node-Matches");
                    return;
                }

                matches.forEach(function(match, matchIndex)
                {
                    consoledebug(
                        `    [Match ${matchIndex}] prefix6_hex=${match.prefix6_hex || ""}, name=${match.name || ""}, adv_lat_e6=${match.adv_lat_e6}, adv_lon_e6=${match.adv_lon_e6}`
                    );
                });
            });

            const resolvedPath = resolvePathGreedyFromEndpoint(pathEntry, endpoint);
            debugPrintResolvedPath(resolvedPath, endpoint);
        });

        const normalizedResolvedPaths = buildResolvedPathList(paths, endpoint);
        const preferredResolvedPath = selectPreferredResolvedPath(normalizedResolvedPaths);

        resolvedPathsByCorrelationKey[key] =
        {
            all_paths: normalizedResolvedPaths,
            preferred_path: preferredResolvedPath
        };

        consoledebug("ResolvedPathList (object):", normalizedResolvedPaths);

        try
        {
            const jsonPretty = JSON.stringify(normalizedResolvedPaths, null, 2);
            const jsonCompact = JSON.stringify(normalizedResolvedPaths);

            //console.debug("ResolvedPathList (pretty JSON):\n" + jsonPretty);
            //console.debug("ResolvedPathList (compact JSON):", jsonCompact);
        }
        catch (error)
        {
            console.error("JSON stringify fehlgeschlagen:", error);
        }

        resolvedPathsByCorrelationKey[key] =
        {
            all_paths: normalizedResolvedPaths,
            preferred_path: preferredResolvedPath
        };
        if (!preferredResolvedPath)
        {
            hideChatPathMap();
            return;
        }
        debugPrintPreferredResolvedPath(preferredResolvedPath);
        showPreferredPathInPathMap(key, preferredResolvedPath);
    }
    catch (error)
    {
        console.error("Pfadabfrage fehlgeschlagen:", error);
    }
}

function renderChatMessages(messages)
{
    if (!el.chatBody)
    {
        return;
    }

    //const shouldAutoScroll = isChatNearBottom();
    state.chatMessages = Array.isArray(messages) ? messages : [];

    if (state.chatMessages.length === 0)
    {
        el.chatBody.innerHTML =
            '<div class="chat-empty">' +
            escapeHtml(tr("chat.empty", "Keine Messages für diesen Eintrag gefunden.")) +
            '</div>';

        /*if (shouldAutoScroll)
        {
            scrollChatToBottom();
        }*/

        return;
    }

    const html = state.chatMessages.map(function(msg)
    {
        return Number(msg.direction || 0) === 1
            ? renderOutgoingMessage(msg)
            : renderIncomingMessage(msg);
    }).join("");

    el.chatBody.innerHTML = `<div class="chat-messages">${html}</div>`;
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

    const oldScrollTop = el.chatBody.scrollTop;
    const oldScrollHeight = el.chatBody.scrollHeight;

    const data = await MeshCoreApi.loadMessages(
    {
        kind: chatKind,
        name: chatName,
        channel_key_hex: row.key_hex || ""
    });

    const messages = data.messages || [];
    const newestId = messages.length > 0 ? Number(messages[messages.length - 1].id || 0) : 0;
    const newestMessageEpoch = messages.length > 0
        ? Number(messages[messages.length - 1].timestamp_epoch || 0)
        : 0;
    const hasNewMessages = newestId > state.chatLastMessageId;
    const oldSerialized = JSON.stringify(state.chatMessages);
    const newSerialized = JSON.stringify(messages);

    if (oldSerialized === newSerialized)
    {
        return;
    }

    renderChatMessages(messages);
    state.chatLastMessageId = newestId;

    if (chatKind === "channel" && newestMessageEpoch > 0)
    {
        markChannelAsRead(
        {
            key_hex: row.key_hex,
            newest_message_epoch: newestMessageEpoch
        });

        renderChannelsList();
    }
    else if (newestMessageEpoch > 0)
    {
        markContactAsRead(
        {
            name: row.name,
            advert_type: row.advert_type,
            advert_type_label: row.advert_type_label,
            newest_msg_epoch: newestMessageEpoch
        });

        if (table)
        {
            table.redraw(true);
        }
    }

    requestAnimationFrame(function()
    {
        if (!keepScrollIfPossible)
        {
            scrollChatToBottom();
            return;
        }

        if (wasNearBottom || hasNewMessages)
        {
            scrollChatToBottom();
            return;
        }

        const newScrollHeight = el.chatBody.scrollHeight;
        const heightDiff = newScrollHeight - oldScrollHeight;
        el.chatBody.scrollTop = oldScrollTop + heightDiff;
    });
}


function getIncomingMessageChatLabel(msg)
{
    const chatKind = Number(msg.chat_kind || 0);

    if (chatKind === 2)
    {
        if (msg.channel_key_hex)
        {
            const channel = state.channels.find(function(item)
            {
                return String(item.key_hex || "").toUpperCase() === String(msg.channel_key_hex || "").toUpperCase();
            });

            if (channel && channel.name)
            {
                return `Channel ${channel.name}`;
            }
        }

        return msg.name ? `Channel ${msg.name}` : "Channel";
    }

    if (chatKind === 1)
    {
        return `Room ${msg.name || "?"}`;
    }

    return `DM ${msg.name || "?"}`;
}

async function pollIncomingMessages()
{
    if (state.incomingMessagePollActive)
    {
        return;
    }

    state.incomingMessagePollActive = true;

    try
    {
        const data = await MeshCoreApi.loadNewMessages(state.lastIncomingMessageId);

        const messages = Array.isArray(data.messages) ? data.messages : [];

        if (!state.incomingMessageDetectionReady)
        {
            state.lastIncomingMessageId = Number(data.newest_id || 0);
            state.incomingMessageDetectionReady = true;
            return;
        }

        if (messages.length > 0)
        {
            console.info("Neue MeshCore RX-Message");
            playNotificationBeep();
        }

        state.lastIncomingMessageId = Number(data.newest_id || state.lastIncomingMessageId);
    }
    catch (error)
    {
        console.warn("Incoming message detection failed", error);
    }
    finally
    {
        state.incomingMessagePollActive = false;
    }
}

async function playBeepTone(ctx, frequency, duration, volume)
{
    const oscillator = ctx.createOscillator();
    const gainNode = ctx.createGain();

    const now = ctx.currentTime;

    oscillator.type = "sine";
    oscillator.frequency.setValueAtTime(frequency, now);

    gainNode.gain.setValueAtTime(0.0001, now);
    gainNode.gain.exponentialRampToValueAtTime(volume, now + 0.01);
    gainNode.gain.exponentialRampToValueAtTime(0.0001, now + duration);

    oscillator.connect(gainNode);
    gainNode.connect(ctx.destination);

    oscillator.start(now);
    oscillator.stop(now + duration + 0.03);

    await new Promise(function(resolve)
    {
        oscillator.addEventListener("ended", resolve, { once: true });
    });

    oscillator.disconnect();
    gainNode.disconnect();
}

function delay(ms)
{
    return new Promise(function(resolve)
    {
        setTimeout(resolve, ms);
    });
}

async function playNotificationBeep()
{
    try
    {
        if (!state.notificationAudioEnabled)
        {
            return;
        }

        if (notificationAudioContext === null)
        {
            notificationAudioContext = new (
                window.AudioContext ||
                window.webkitAudioContext
            )();
        }

        const ctx = notificationAudioContext;

        if (ctx.state === "suspended")
        {
            await ctx.resume();
            await delay(50);
        }

        if (ctx.state !== "running")
        {
            console.warn("Beep skipped, AudioContext state:", ctx.state);
            return;
        }

        await playBeepTone(ctx, 600, 0.18, 0.35);
        await delay(100);
        await playBeepTone(ctx, 900, 0.28, 0.7);
    }
    catch (error)
    {
        console.warn("Beep failed", error);
    }
}

function startIncomingMessageDetection()
{
    if (state.incomingMessagePollTimer)
    {
        return;
    }

    pollIncomingMessages().catch(function(error)
    {
        console.error("Initiale RX-Message-Erkennung fehlgeschlagen:", error);
    });

    state.incomingMessagePollTimer = setInterval(function()
    {
        pollIncomingMessages().catch(function(error)
        {
            console.error("RX-Message-Erkennung fehlgeschlagen:", error);
        });
    }, 1000);
}

function startCurrentChatRefresh()
{
    stopCurrentChatRefresh();

    state.chatRefreshTimer = setInterval(async function()
    {
        if (
            !state.chatRow ||
            !el.chatTabsView ||
            (state.rightView !== "chat" && state.rightView !== "pathmap")
        )
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
    setRightPanelMode("messages", null);
    state.chatRow = row;
    state.chatLastMessageId = 0;
    state.chatMessages = [];
    setChatInputEnabled(true);

    if (!row || !el.chatView || !el.chatTitle || !el.chatBody)
    {
        return;
    }

    hideRightPanelViews();
    hideChatPathMap();

    const chatName = row.name || getChatKindLabel(row);
    el.chatTitle.textContent = `${getChatKindLabel(row)}: ${chatName}`;
    el.chatBody.innerHTML =
        '<div class="chat-empty">' +
        escapeHtml(tr("chat.loading", "Lade Messages ...")) +
        '</div>';

    if (el.chatTabsView)
    {
        el.chatTabsView.style.display = "flex";
    }

    activateChatTab("messages");

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
    return await MeshCoreApi.saveRoomPassword(context, password);
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
            const data = await MeshCoreApi.loadTxStatus(txId);

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
            channel_key_hex: String(row.key_hex || ""),
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
        const result = await MeshCoreApi.sendFloodAdvert();

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
    const tabMonitorBtn = document.getElementById("tabMonitorBtn");
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

    if (tabMonitorBtn)
    {
        tabMonitorBtn.classList.toggle("active", tabName === "monitor");
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
        if (!!a.is_default !== !!b.is_default)
        {
            return a.is_default ? -1 : 1;
        }

        if (!!a.has_local_context !== !!b.has_local_context)
        {
            return a.has_local_context ? -1 : 1;
        }

        const nameA = String(a.name || "");
        const nameB = String(b.name || "");
        const byName = nameA.localeCompare(nameB, undefined, { sensitivity: "base" });

        if (byName !== 0)
        {
            return byName;
        }

        return Number(a.channel_idx || 0) - Number(b.channel_idx || 0);
    });

    state.channels.forEach(function(channel)
    {
        const btn = document.createElement("button");
        btn.type = "button";
        btn.className = "channel-item";

        const newestMessageEpoch = Number(channel.newest_message_epoch || 0);
        const lastReadEpoch = getChannelLastReadEpoch(channel);

        if (newestMessageEpoch > lastReadEpoch)
        {
            btn.classList.add("channel-has-unread");
        }

        const channelIdx = Number(channel.channel_idx || 0);
        const channelName =
            (channel.name && String(channel.name).trim() !== "")
                ? String(channel.name)
                : (tr("channel.fallback_name", "Channel {idx}", { idx: channelIdx }));

        if (
            state.chatRow &&
            state.chatRow.type === "channel" &&
            String(state.chatRow.key_hex || "") === String(channel.key_hex || "")
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
    
        if (channel.has_local_context)
        {
            metaParts.push(tr("channel.meta.local", "local"));
        }

        const messageCount = Number(channel.message_count || 0);
        const messageLabel = messageCount === 1
            ? tr("channel.meta.message_singular", "Nachricht")
            : tr("channel.meta.message_plural", "Nachrichten");

        metaParts.push(String(messageCount) + " " + messageLabel);

        const metaEl = document.createElement("span");
        metaEl.className = "channel-item-meta";

        if (messageCount > 0)
        {
            metaEl.classList.add("channel-has-messages");
        }

        metaEl.textContent = metaParts.join(" • ");
        btn.appendChild(metaEl);

        btn.addEventListener("click", function()
        {
            state.chatRow =
            {
                type: "channel",
                name: channelName,
                key_hex: String(channel.key_hex || ""),
                channel_idx: channelIdx,
                enabled: !!channel.enabled,
                is_observed: !!channel.is_observed,
                has_local_context: !!channel.has_local_context,
                is_default: !!channel.is_default,
                newest_message_epoch: newestMessageEpoch
            };

            markChannelAsRead(state.chatRow);

            setLeftTab("channels");
            showChatForRow(state.chatRow);
            renderChannelsList();
        });

        listEl.appendChild(btn);
    });
}

async function loadChannels()
{
    const data = await MeshCoreApi.loadChannels();

    state.channels = Array.isArray(data.channels) ? data.channels : [];

    updateLeftCounter();
}

async function sendCurrentChatMessage()
{
    if (!state.chatRow || !el.chatInput)
    {
        return;
    }

    const messageText = getChatInputText().trim();

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

        if (!state.chatRow.has_local_context)
        {
            window.alert(
                tr(
                    "channel.send.no_local_context",
                    "Für diesen Channel ist kein lokaler Sendekontext konfiguriert."
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

    let sendSucceeded = false;
    try
    {
        const data = await MeshCoreApi.sendMessage(buildOutgoingPayload(state.chatRow, messageText));

        clearChatInput();
        await loadChatMessages(state.chatRow, false);

        if (data.id)
        {
            startTxStatusPolling(data.id);
        }
        sendSucceeded = true;
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
        if (sendSucceeded)
        {
            scrollChatToBottom();
        }
    }
}

async function resendChatMessageById(messageId)
{
    if (!state.chatRow || !isChatLikeNode(state.chatRow))
    {
        return;
    }

    const msg = state.chatMessages.find(function(item)
    {
        return String(item.id || "") === String(messageId || "");
    });

    if (!msg)
    {
        return;
    }

    const messageText = String(msg.text || msg.message_text || "").trim();

    if (messageText === "")
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

        if (!state.chatRow.has_local_context)
        {
            window.alert(
                tr(
                    "channel.send.no_local_context",
                    "Für diesen Channel ist kein lokaler Sendekontext konfiguriert."
                )
            );
            return;
        }
    }

    try
    {
        const data = await MeshCoreApi.sendMessage(buildOutgoingPayload(state.chatRow, messageText));

        await loadChatMessages(state.chatRow, false);

        if (data.id)
        {
            startTxStatusPolling(data.id);
        }

        scrollChatToBottom();
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
                    markContactAsRead(row);
                    showChatForRow(row);
                    table.redraw(true);
                }
                else if (isRepeaterNode(row))
                {
                    showRepeaterInfo(row);
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

                state.autoZoom = true;

                if (hasLocation(row))
                {
                    showMapForRow(row);
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

            //let timestamp = parseMariaDbDateTime(node.last_advert_at);
            let timestamp = normalizeGuiTimestamp(parseMariaDbDateTime(node.last_advert_at));

            timestamp = normalizeGuiTimestamp(timestamp);

            if (!timestamp || timestamp <= 0)
            {
                return false;
            }

            const diff = Math.floor((Date.now() - timestamp) / 1000)

            if (el.callsignFilter && el.callsignFilter.checked)
            {
                if (!containsPossibleCallsign(node.name))
                {
                    return false;
                }
            }

            if (el.activeFilter && el.activeFilter.checked)
            {
                if (!isChatLikeNode(node) || Number(node.msg_count || 0) < 1)
                {
                    return false;
                }
            }

            //if (el.localFilter && el.localFilter.checked)
            {
                if (Number(node.is_local || 0) !== 1)
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
    return await MeshCoreApi.saveChannel(payload);
}

async function deleteChannel(keyHex)
{
    return await MeshCoreApi.deleteChannel(keyHex);
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

            await deleteChannel(selected.key_hex);

            if (
                state.chatRow &&
                isChannelRow(state.chatRow) &&
                String(state.chatRow.key_hex || "") === String(selected.key_hex || "")
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
                key_hex: String(result.channel.key_hex || ""),
                channel_idx: Number(result.channel.channel_idx || 0),
                enabled: !!result.channel.enabled,
                is_observed: !!result.channel.is_observed,
                has_local_context: !!result.channel.has_local_context,
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
    const tabMonitorBtn = document.getElementById("tabMonitorBtn");

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

    if (tabMonitorBtn)
    {
        tabMonitorBtn.addEventListener("click", function()
        {
            setLeftTab("monitor");
            showMonitor();
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

    if (el.setupCityInput)
    {
        el.setupCityInput.value = "";
    }

    if (el.setupLatInput)
    {
        el.setupLatInput.value = "";
    }

    if (el.setupLonInput)
    {
        el.setupLonInput.value = "";
    }

    if (el.setupBotInput)
    {
        el.setupBotInput.checked = false;
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

            if (el.setupCityInput)
            {
                el.setupCityInput.value = cfg.location_name || "";
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

            if (el.setupBotInput)
            {
                el.setupBotInput.checked = cfg.bot === true || Number(cfg.bot) === 1;
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
    return await MeshCoreApi.loadCompanionSetup();
}

async function applyCompanionSetup()
{
    const name = el.setupNameInput ? el.setupNameInput.value.trim() : "";
    const locationName = el.setupCityInput ? el.setupCityInput.value.trim() : "";
    const setupData = await loadCompanionSetup();
    const cfg = setupData && setupData.config ? setupData.config : null;
    const homeRepeaterName = cfg && cfg.protected_repeater_name ? String(cfg.protected_repeater_name).trim() : "";
    const latText = el.setupLatInput ? el.setupLatInput.value.trim() : "";
    const lonText = el.setupLonInput ? el.setupLonInput.value.trim() : "";
    const botEnabled = el.setupBotInput ? el.setupBotInput.checked : false;

    const latitude = Number(latText);
    const longitude = Number(lonText);

    if (name === "")
    {
        throw new Error(tr("setup.error.name_missing", "Name fehlt."));
    }

    if (locationName.length > 128)
    {
        throw new Error(tr("setup.error.city_too_long", "City ist zu lang."));
    }

    if (homeRepeaterName.length > 64)
    {
        throw new Error(tr("setup.error.home_repeater_too_long", "Home Repeater ist zu lang."));
    }

    if (!Number.isFinite(latitude) || latitude < -90 || latitude > 90)
    {
        throw new Error(tr("setup.error.latitude_invalid", "Latitude ist ungültig."));
    }

    if (!Number.isFinite(longitude) || longitude < -180 || longitude > 180)
    {
        throw new Error(tr("setup.error.longitude_invalid", "Longitude ist ungültig."));
    }

    return await MeshCoreApi.applyCompanionSetup(
    {
        name: name,
        location_name: locationName,
        protected_repeater_name: homeRepeaterName,
        latitude: latitude,
        longitude: longitude,
        bot: botEnabled
    });
}

async function resetNodePath(publicKeyHex)
{
    return await MeshCoreApi.resetNodePath(publicKeyHex);
}

async function handleResetPathButtonClick()
{
    const row = state.mapContextRow;

    if (!row)
    {
        return;
    }

    const publicKeyHex = String(row.public_key_hex || "").trim();
    const nodeName = String(row.name || "").trim() || "-";

    if (!/^[0-9A-Fa-f]{64}$/.test(publicKeyHex))
    {
        window.alert(
            tr(
                "map.reset_path.invalid_key",
                "Für diesen Eintrag ist kein gültiger Public Key vorhanden."
            )
        );
        return;
    }

    const confirmed = window.confirm(
        tr(
            "map.reset_path.confirm",
            "Gespeicherten Pfad für \"{name}\" löschen?",
            {
                name: nodeName
            }
        )
    );

    if (!confirmed)
    {
        return;
    }

    state.resetPathPending = true;
    updateMapActionButtons();

    try
    {
        await resetNodePath(publicKeyHex);
        //window.alert(`Pfad für "${nodeName}" wurde zum Löschen vorgemerkt.`);
    }
    catch (error)
    {
        window.alert(
            tr(
                "map.reset_path.error",
                "Fehler beim Löschen des Pfads: {message}",
                {
                    message: error.message || tr("error.unknown", "Unbekannter Fehler")
                }
            )
        );
    }
    finally
    {
        state.resetPathPending = false;
        updateMapActionButtons();
    }
}

if (el.resetPathButton)
{
    el.resetPathButton.textContent = tr("map.reset_path", "Pfad löschen");
}

el.mapPositionButton?.addEventListener("click", function()
{
    if (state.mapContextRow)
    {
        showMapForRow(state.mapContextRow);
    }
});

el.mapPathButton?.addEventListener("click", function()
{
    if (state.mapContextRow)
    {
        showNodeAdvertPath(state.mapContextRow);
    }
});

el.resetPathButton?.addEventListener("click", function()
{
    handleResetPathButtonClick();
});

if (el.protectedRepeaterButton)
{
    el.protectedRepeaterButton.addEventListener("click", async function()
    {
        await openProtectedRepeaterDialog();
    });
}

if (el.protectedRepeaterStartButton)
{
    el.protectedRepeaterStartButton.addEventListener("click", handleProtectedRepeaterStartClick);
}

if (el.protectedRepeaterCloseButton)
{
    el.protectedRepeaterCloseButton.addEventListener("click", function()
    {
        closeProtectedRepeaterDialog();
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

if (el.activeFilter)
{
    el.activeFilter.addEventListener("change", function()
    {
        el.tableError.innerHTML = "";
        table.replaceData();
    });
}
/*
if (el.localFilter)
{
    el.localFilter.addEventListener("change", function()
    {
        el.tableError.innerHTML = "";
        table.replaceData();
    });
}
*/
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

    el.chatInput.addEventListener("input", function()
    {
        updateChatInputHighlight();
    });

    el.chatInput.addEventListener("paste", function(e)
    {
        e.preventDefault();

        const pastedText = (e.clipboardData || window.clipboardData).getData("text");
        document.execCommand("insertText", false, pastedText.replace(/[\r\n]+/g, " "));
    });

    updateChatInputHighlight();
}

if (el.chatBody)
{
    el.chatBody.addEventListener("click", function(event)
    {
        const resendButton = event.target.closest(".chat-resend-button");

        if (resendButton)
        {
            const messageId = resendButton.getAttribute("data-message-id") || "";
            resendChatMessageById(messageId);
            return;
        }

        const replyButton = event.target.closest(".chat-reply-button");

        if (replyButton)
        {
            const name = replyButton.getAttribute("data-reply-name") || "";
            insertReplyMention(name);
            return;
        }

        const pathButton = event.target.closest(".chat-path-button");

        if (pathButton)
        {
            const correlationKey = pathButton.getAttribute("data-correlation-key") || "";
            handleMessagePathClick(correlationKey);
        }
    });
}

if (el.chatSymbolButton && el.chatSymbolPalette)
{
    buildChatSymbolPalette();

    el.chatSymbolButton.addEventListener("click", function(event)
    {
        event.stopPropagation();
        toggleChatSymbolPalette();
    });

    el.chatSymbolPalette.addEventListener("click", function(event)
    {
        const symbolButton = event.target.closest(".chat-symbol-item");

        if (!symbolButton)
        {
            return;
        }

        insertTextIntoChatInput(symbolButton.getAttribute("data-symbol") || "");
    });

    document.addEventListener("click", function(event)
    {
        if (event.target === el.chatSymbolButton ||
            el.chatSymbolButton.contains(event.target) ||
            el.chatSymbolPalette.contains(event.target))
        {
            return;
        }

        setChatSymbolPaletteVisible(false);
    });

    document.addEventListener("keydown", function(event)
    {
        if (event.key === "Escape")
        {
            setChatSymbolPaletteVisible(false);
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

el.chatPathToggleNames?.addEventListener("change", function()
{
    state.chatPathShowNames = !!el.chatPathToggleNames.checked;
    syncPathDisplayCheckboxes();
    savePathDisplaySettings();

    if (state.chatPathLastPreferredPath)
    {
        showPreferredPathInPathMap(
            state.chatPathLastCorrelationKey,
            state.chatPathLastPreferredPath
        );
    }
});

el.chatPathToggleHash?.addEventListener("change", function()
{
    state.chatPathShowHash = !!el.chatPathToggleHash.checked;
    syncPathDisplayCheckboxes();
    savePathDisplaySettings();

    if (state.chatPathLastPreferredPath)
    {
        showPreferredPathInPathMap(
            state.chatPathLastCorrelationKey,
            state.chatPathLastPreferredPath
        );
    }
});

el.chatPathToggleDistances?.addEventListener("change", function()
{
    state.chatPathShowDistances = !!el.chatPathToggleDistances.checked;
    syncPathDisplayCheckboxes();
    savePathDisplaySettings();

    if (state.chatPathLastPreferredPath)
    {
        showPreferredPathInPathMap(
            state.chatPathLastCorrelationKey,
            state.chatPathLastPreferredPath
        );
    }
});

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

if (el.channelActionSelect)
{
    el.channelActionSelect.addEventListener("change", function()
    {
        const action = el.channelActionSelect.value;

        if (!action)
        {
            return;
        }

        openChannelDialog(action);

        el.channelActionSelect.value = "";
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

el.tabMessagesViewBtn?.addEventListener("click", function()
{
    if (!state.chatRow)
    {
        return;
    }

    state.rightView = "chat";
    activateChatTab("messages");
});

el.tabPathMapViewBtn?.addEventListener("click", function()
{
    if (!state.chatRow || !state.chatPathLastPreferredPath)
    {
        return;
    }

    state.rightView = "pathmap";
    activateChatTab("pathmap");
});

el.mapPathToggleNames?.addEventListener("change", function()
{
    state.chatPathShowNames = !!el.mapPathToggleNames.checked;
    syncPathDisplayCheckboxes();
    savePathDisplaySettings();

    if (state.lastNodeAdvertPathRow)
    {
        showNodeAdvertPath(state.lastNodeAdvertPathRow);
    }
});

el.mapPathToggleHash?.addEventListener("change", function()
{
    state.chatPathShowHash = !!el.mapPathToggleHash.checked;
    syncPathDisplayCheckboxes();
    savePathDisplaySettings();

    if (state.lastNodeAdvertPathRow)
    {
        showNodeAdvertPath(state.lastNodeAdvertPathRow);
    }
});

el.mapPathToggleDistances?.addEventListener("change", function()
{
    state.chatPathShowDistances = !!el.mapPathToggleDistances.checked;
    syncPathDisplayCheckboxes();
    savePathDisplaySettings();

    if (state.lastNodeAdvertPathRow)
    {
        showNodeAdvertPath(state.lastNodeAdvertPathRow);
    }
});

syncPathDisplayCheckboxes();
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

syncNotificationAudioCheckbox();

el.notificationAudioToggle?.addEventListener("change", function()
{
    state.notificationAudioEnabled = !!el.notificationAudioToggle.checked;
    saveNotificationAudioEnabled();
});

let notificationAudioContext = null;

async function unlockNotificationAudio()
{
    try
    {
        if (notificationAudioContext === null)
        {
            notificationAudioContext = new (
                window.AudioContext ||
                window.webkitAudioContext
            )();
        }

        if (notificationAudioContext.state === "suspended")
        {
            await notificationAudioContext.resume();
            await delay(50);
        }

        if (notificationAudioContext.state === "running")
        {
            await playBeepTone(notificationAudioContext, 300, 0.05, 0.02);
        }
    }
    catch (error)
    {
        console.warn("Notification audio unlock failed", error);
    }
}

document.addEventListener("click", unlockNotificationAudio);
document.addEventListener("keydown", unlockNotificationAudio);
document.addEventListener("touchstart", unlockNotificationAudio);

refreshNoiseFloor();
startIncomingMessageDetection();

state.noiseFloorRefreshTimer = setInterval(function()
{
    refreshNoiseFloor();
}, 3000);

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

