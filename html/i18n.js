const I18N =
{
    de:
    {
        "html.lang": "de",
        "document.title": "MeshCore Dashboard",
        "page.title": "MeshCore Web-Dashboard",
        "page.subtitle": "by DJ0ABR",

        "panel.contacts_channels": "Kontakte/Kanäle",
        "panel.sort_hint": "sortierbar per Klick auf den Spaltenkopf",
        "panel.messages": "Messages",

        "toolbar.type": "Typ:",
        "toolbar.filter.all": "Alle",
        "toolbar.filter.chat": "Chat",
        "toolbar.filter.repeater": "Repeater",
        "toolbar.filter.room": "Raum",
        "toolbar.filter.sensor": "Sensor",
        "toolbar.all_map": "Karte Alle",
        "toolbar.advert": "Advert",

        "tabs.contacts": "Kontakte",
        "tabs.channels": "Kanäle",

        "chat.title.default": "Chat",
        "chat.input_placeholder": "Nachricht eingeben und Enter drücken ...",
        "chat.send": "Senden",
        "chat.empty": "Keine Messages für diesen Eintrag gefunden.",
        "chat.loading": "Lade Messages ...",
        "chat.error_loading": "Fehler beim Laden der Messages: {message}",
        "chat.refresh_failed": "Chat-Refresh fehlgeschlagen: {message}",
        "chat.status.pending": "Ausstehend",
        "chat.you": "Du",
        "chat.snr": "SNR",
        "chat.path": "Pfad",
        "chat.send_error": "Fehler beim Senden: {message}",

        "map.empty.default": "Linksklick auf Chat, Raum oder Kanal = Verlauf, Rechtsklick = Karte.",
        "map.empty.no_nodes": "Keine Nodes mit Position vorhanden.",
        "map.no_position_for_node": "keine Positionsdaten verfügbar.",

        "room_password.title": "Passwort für Raum",
        "room_password.title_with_name": "Passwort für Raum {name}",
        "room_password.label": "Raum Passwort",
        "room_password.subtitle.required": "Das Backend meldet: Raum Password erforderlich",
        "room_password.cancel": "Abbrechen",
        "room_password.save": "Speichern",
        "room_password.error.empty": "Bitte ein Passwort eingeben.",
        "room_password.error.save_failed": "Speichern fehlgeschlagen.",

        "channel.modal.base_title": "Kanal",
        "channel.name": "Kanalname",
        "channel.secret": "Secret Key",
        "channel.generated_secret": "Generierter Secret Key",
        "channel.qr_code": "QR-Code",
        "channel.ok": "OK",
        "channel.cancel": "Abbrechen",
        "channel.none_selected": "Kein Kanal ausgewählt.",
        "channel.invalid_action": "Ungültige Aktion.",
        "channel.fallback_name": "Kanal {idx}",
        "channel.meta.default": "Standard",
        "channel.meta.observed": "beobachtet",
        "channel.meta.disabled": "deaktiviert",
        "channel.send.disabled": "Dieser Kanal ist deaktiviert.",
        "channel.send.observed_no_context": "Für diesen beobachteten Kanal ist kein lokaler Sendekontext konfiguriert.",

        "channel.action.create_private.title": "Privaten Kanal anlegen",
        "channel.action.create_private.subtitle": "Neuen privaten Kanal anlegen. Der Secret Key wird anschließend angezeigt.",
        "channel.action.create_private.confirm": "Erstellen",
        "channel.action.create_private.done_subtitle": "Kanal wurde erstellt. Diesen Secret Key jetzt mit den anderen Teilnehmern teilen.",

        "channel.action.join_private.title": "Privatem Kanal beitreten",
        "channel.action.join_private.subtitle": "Kanalname und Secret Key eingeben.",
        "channel.action.join_private.confirm": "Beitreten",

        "channel.action.join_public.title": "Öffentlichem Kanal beitreten",
        "channel.action.join_public.subtitle": "Öffentlichem Kanal über den Namen beitreten.",
        "channel.action.join_public.confirm": "Beitreten",

        "channel.action.join_hashtag.title": "Hashtag-Kanal beitreten",
        "channel.action.join_hashtag.subtitle": "Hashtag-Kanal eingeben, z. B. #drones.",
        "channel.action.join_hashtag.confirm": "Beitreten",

        "channel.action.remove.title": "Kanal entfernen",
        "channel.action.remove.subtitle": "Kanal wirklich entfernen: {name} (IDX {idx})",
        "channel.action.remove.none_selected": "Kein Kanal ausgewählt.",
        "channel.action.remove.confirm": "Entfernen",

        "channel.button.create_private": "Privat erstellen",
        "channel.button.join_private": "Privat beitreten",
        "channel.button.join_public": "Öffentlich beitreten",
        "channel.button.join_hashtag": "Hashtag beitreten",
        "channel.button.remove": "Entfernen",

        "channel.type.chat": "Chat",
        "channel.type.room": "Raum",
        "channel.type.channel": "Kanal",
        "channel.kind.dm": "dm",
        "channel.kind.room": "room",
        "channel.kind.channel": "channel",

        "channels.none": "Keine Kanäle vorhanden",

        "discover.title": "Repeater finden",
        "discover.status.label": "Status",
        "discover.status.unknown": "Unbekannt",
        "discover.job.label": "Letzter Job",
        "discover.job.none": "Noch keine Suche gestartet.",
        "discover.results.label": "Gefundene Repeater",
        "discover.results.none": "Noch keine Ergebnisse.",
        "discover.button.start": "START",
        "discover.button.close": "Schließen",
        "discover.error.start_failed": "Suche konnte nicht gestartet werden.",
        "discover.error.status_failed": "Such-Status konnte nicht geladen werden.",
        "discover.status.queued": "Warteschlange",
        "discover.status.running": "läuft",
        "discover.status.done": "fertig",
        "discover.status.failed": "Fehler",
        "discover.status.skipped": "übersprungen",
        "discover.status.unknown_fallback": "unbekannt",
        "discover.updated": "Update",
        "discover.results_count": "Treffer",
        "discover.finished": "beendet",
        "discover.started": "gestartet",
        "discover.created": "angelegt",
        "discover.error_label": "Fehler",

        "setup.title": "Companion Setup",
        "setup.name": "Name",
        "setup.latitude": "Breitegrad",
        "setup.longitude": "Längengrad",
        "setup.fixed_radio": "Feste Funkparameter",
        "setup.apply": "Anwenden",
        "setup.cancel": "Abbrechen",
        "setup.error.name_missing": "Name fehlt.",
        "setup.error.latitude_invalid": "Breitegrad ist ungültig.",
        "setup.error.longitude_invalid": "Längengrad ist ungültig.",
        "setup.error.load_failed": "Setup-Werte konnten nicht geladen werden.",
        "setup.error.apply_failed": "Setup fehlgeschlagen.",

        "type.chat": "Chat",
        "type.repeater": "Repeater",
        "type.room": "Raum",
        "type.sensor": "Sensor",
        "type.unknown": "Unbekannt",

        "table.placeholder.no_nodes": "Keine Nodes gefunden.",
        "table.column.type": "Typ",
        "table.column.name": "Name",
        "table.column.last": "Zuletzt",

        "counter.channels_observed": "{count} Kanäle ({observed} beobachtet)",
        "counter.channels": "{count} Kanäle",
        "counter.nodes": "{count} Nodes",

        "status.error": "Fehler",

        "common.done": "Fertig",
        "common.cancel": "Abbrechen",
        "common.save": "Speichern",

        "error.unknown": "Unbekannter Fehler",
        "error.qr_library_missing": "QR-Code Bibliothek nicht geladen.",
        "error.loading": "Fehler beim Laden: {message}",
        "error.invalid_json": "keine gültige JSON-Antwort",
        "error.http": "HTTP Fehler",
        "error.generic": "Fehler",
        "room_password.input_placeholder": "Passwort eingeben ..."
    },

    en:
    {
        "html.lang": "en",
        "document.title": "MeshCore Dashboard",
        "page.title": "MeshCore Web Dashboard",
        "page.subtitle": "by DJ0ABR",

        "panel.contacts_channels": "Contacts/Channels",
        "panel.sort_hint": "sortable by clicking the column header",
        "panel.messages": "Messages",

        "toolbar.type": "Type:",
        "toolbar.filter.all": "All",
        "toolbar.filter.chat": "Chat",
        "toolbar.filter.repeater": "Repeater",
        "toolbar.filter.room": "Room",
        "toolbar.filter.sensor": "Sensor",
        "toolbar.all_map": "All Map",
        "toolbar.advert": "Advert",

        "tabs.contacts": "Contacts",
        "tabs.channels": "Channels",

        "chat.title.default": "Chat",
        "chat.input_placeholder": "Enter message and press Enter ...",
        "chat.send": "Send",
        "chat.empty": "No messages found for this entry.",
        "chat.loading": "Loading messages ...",
        "chat.error_loading": "Error loading messages: {message}",
        "chat.refresh_failed": "Chat refresh failed: {message}",
        "chat.status.pending": "Pending",
        "chat.you": "You",
        "chat.snr": "SNR",
        "chat.path": "Path",
        "chat.send_error": "Error sending message: {message}",

        "map.empty.default": "Left click on chat, room or channel = history, right click = map.",
        "map.empty.no_nodes": "No nodes with position available.",
        "map.no_position_for_node": "no position data available.",

        "room_password.title": "Password for room",
        "room_password.title_with_name": "Password for room {name}",
        "room_password.label": "Room password",
        "room_password.subtitle.required": "The backend reports: room password required",
        "room_password.cancel": "Cancel",
        "room_password.save": "Save",
        "room_password.error.empty": "Please enter a password.",
        "room_password.error.save_failed": "Saving failed.",

        "channel.modal.base_title": "Channel",
        "channel.name": "Channel name",
        "channel.secret": "Secret key",
        "channel.generated_secret": "Generated secret key",
        "channel.qr_code": "QR code",
        "channel.ok": "OK",
        "channel.cancel": "Cancel",
        "channel.none_selected": "No channel selected.",
        "channel.invalid_action": "Invalid action.",
        "channel.fallback_name": "Channel {idx}",
        "channel.meta.default": "default",
        "channel.meta.observed": "observed",
        "channel.meta.disabled": "disabled",
        "channel.send.disabled": "This channel is disabled.",
        "channel.send.observed_no_context": "No local sending context is configured for this observed channel.",

        "channel.action.create_private.title": "Create private channel",
        "channel.action.create_private.subtitle": "Create a new private channel. The secret key will be shown afterwards.",
        "channel.action.create_private.confirm": "Create",
        "channel.action.create_private.done_subtitle": "Channel created. Share this secret key with the other participants now.",

        "channel.action.join_private.title": "Join private channel",
        "channel.action.join_private.subtitle": "Enter channel name and secret key.",
        "channel.action.join_private.confirm": "Join",

        "channel.action.join_public.title": "Join public channel",
        "channel.action.join_public.subtitle": "Join a public channel by name.",
        "channel.action.join_public.confirm": "Join",

        "channel.action.join_hashtag.title": "Join hashtag channel",
        "channel.action.join_hashtag.subtitle": "Enter a hashtag channel, e.g. #drones.",
        "channel.action.join_hashtag.confirm": "Join",

        "channel.action.remove.title": "Remove channel",
        "channel.action.remove.subtitle": "Really remove channel: {name} (IDX {idx})",
        "channel.action.remove.none_selected": "No channel selected.",
        "channel.action.remove.confirm": "Remove",

        "channel.button.create_private": "Create Private",
        "channel.button.join_private": "Join Private",
        "channel.button.join_public": "Join Public",
        "channel.button.join_hashtag": "Join Hashtag",
        "channel.button.remove": "Remove",

        "channel.type.chat": "Chat",
        "channel.type.room": "Room",
        "channel.type.channel": "Channel",
        "channel.kind.dm": "dm",
        "channel.kind.room": "room",
        "channel.kind.channel": "channel",

        "channels.none": "No channels available",

        "discover.title": "Repeater Discovery",
        "discover.status.label": "Status",
        "discover.status.unknown": "Unknown",
        "discover.job.label": "Last job",
        "discover.job.none": "No discovery has been started yet.",
        "discover.results.label": "Found repeaters",
        "discover.results.none": "No results yet.",
        "discover.button.start": "START",
        "discover.button.close": "Close",
        "discover.error.start_failed": "Failed to start discovery.",
        "discover.error.status_failed": "Failed to load discovery status.",
        "discover.status.queued": "queued",
        "discover.status.running": "running",
        "discover.status.done": "done",
        "discover.status.failed": "failed",
        "discover.status.skipped": "skipped",
        "discover.status.unknown_fallback": "unknown",
        "discover.updated": "Updated",
        "discover.results_count": "Results",
        "discover.finished": "finished",
        "discover.started": "started",
        "discover.created": "created",
        "discover.error_label": "Error",

        "setup.title": "Companion Setup",
        "setup.name": "Name",
        "setup.latitude": "Latitude",
        "setup.longitude": "Longitude",
        "setup.fixed_radio": "Fixed radio parameters",
        "setup.apply": "Apply",
        "setup.cancel": "Cancel",
        "setup.error.name_missing": "Name is missing.",
        "setup.error.latitude_invalid": "Latitude is invalid.",
        "setup.error.longitude_invalid": "Longitude is invalid.",
        "setup.error.load_failed": "Failed to load setup values.",
        "setup.error.apply_failed": "Failed to apply setup.",

        "type.chat": "Chat",
        "type.repeater": "Repeater",
        "type.room": "Room",
        "type.sensor": "Sensor",
        "type.unknown": "Unknown",

        "table.placeholder.no_nodes": "No nodes found.",
        "table.column.type": "Type",
        "table.column.name": "Name",
        "table.column.last": "Last",

        "counter.channels_observed": "{count} channels ({observed} observed)",
        "counter.channels": "{count} channels",
        "counter.nodes": "{count} nodes",

        "status.error": "Error",

        "common.done": "Done",
        "common.cancel": "Cancel",
        "common.save": "Save",

        "error.unknown": "Unknown error",
        "error.qr_library_missing": "QR code library not loaded.",
        "error.loading": "Error loading data: {message}",
        "error.invalid_json": "invalid JSON response",
        "error.http": "HTTP error",
        "error.generic": "Error",
        "room_password.input_placeholder": "Enter password ..."
    }
};

let currentLanguage = localStorage.getItem("dashboardLanguage") || "de";

function getLanguage()
{
    return currentLanguage;
}

function setLanguage(lang)
{
    currentLanguage = I18N[lang] ? lang : "de";
    localStorage.setItem("dashboardLanguage", currentLanguage);
    applyTranslations();

    if (typeof renderChannelsList === "function")
    {
        renderChannelsList();
    }

    if (typeof renderChatMessages === "function")
    {
        renderChatMessages((typeof state !== "undefined" && state && Array.isArray(state.chatMessages)) ? state.chatMessages : []);
    }

    if (typeof renderDiscoverResults === "function")
    {
        renderDiscoverResults(
            (typeof state !== "undefined" && state) ? state.discoverResults : []
        );
    }

    if (typeof updateLeftCounter === "function")
    {
        updateLeftCounter();
    }

    if (typeof updatePageTitle === "function")
    {
        const nodeName =
            (typeof state !== "undefined" && state && state.deviceInfo && state.deviceInfo.node_name)
                ? state.deviceInfo.node_name
                : "";
        updatePageTitle(nodeName);
    }
}

function t(key, vars = {})
{
    const langMap = I18N[currentLanguage] || I18N.de;
    let text = langMap[key] || I18N.de[key] || key;

    Object.keys(vars).forEach(function(name)
    {
        text = text.replaceAll("{" + name + "}", String(vars[name]));
    });

    return text;
}

function applyTranslations()
{
    document.documentElement.lang = t("html.lang");
    document.title = t("document.title");

    document.querySelectorAll("[data-i18n]").forEach(function(node)
    {
        node.textContent = t(node.dataset.i18n);
    });

    document.querySelectorAll("[data-i18n-html]").forEach(function(node)
    {
        node.innerHTML = t(node.dataset.i18nHtml);
    });

    document.querySelectorAll("[data-i18n-placeholder]").forEach(function(node)
    {
        node.placeholder = t(node.dataset.i18nPlaceholder);
    });

    document.querySelectorAll("[data-i18n-title]").forEach(function(node)
    {
        node.title = t(node.dataset.i18nTitle);
    });

    document.querySelectorAll("[data-i18n-value]").forEach(function(node)
    {
        node.value = t(node.dataset.i18nValue);
    });
}

document.addEventListener("DOMContentLoaded", function()
{
    const languageSelect = document.getElementById("languageSelect");

    if (languageSelect)
    {
        languageSelect.value = getLanguage();

        languageSelect.addEventListener("change", function()
        {
            setLanguage(languageSelect.value);
        });
    }

    applyTranslations();
});