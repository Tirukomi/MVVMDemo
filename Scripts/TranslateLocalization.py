"""Fills the German, Japanese and pseudo-locale archives from the tables below.

Run after every GatherText (Scripts/Localize.bat does gather -> translate -> compile). Strings with no entry are left
untranslated on purpose so missing translations show up as English in game rather than as blanks.
"""
import json
import re
import sys

ROOT = r"D:\UEProjects\MVVMSample\Content\Localization\Game"

DE = {
    # V5 combat
    "Counter": "Kontern", "{0}-hit combo": "{0}er-Kombo",
    # V4 menus
    "Blackwater Ops": "Blackwater Ops", "Paused": "Pausiert", "Evidence": "Beweise", "Options": "Optionen",
    "Investigation": "Ermittlung", "Change": "Ändern", "Browse": "Durchsuchen", "Caution": "Achtung",
    "Analysed": "Analysiert", "Not yet found": "Noch nicht gefunden", "No. {0}": "Nr. {0}", "Key bindings": "Tastenbelegung",
    "Display": "Anzeige", "Accessibility": "Barrierefreiheit",
    "Language for all menus, the HUD and subtitles. Changes preview immediately.": "Sprache aller Menüs, des HUD und der Untertitel. Änderungen werden sofort als Vorschau angezeigt.",
    "Swaps the clue, danger and highlight colours for palettes that stay distinct with protanopia, deuteranopia or tritanopia.": "Ersetzt die Farben für Hinweise, Gefahr und Hervorhebungen durch Paletten, die bei Protanopie, Deuteranopie oder Tritanopie unterscheidbar bleiben.",
    "Scales every menu and HUD element. Layouts reflow, so nothing is cut off at larger sizes.": "Skaliert alle Menü- und HUD-Elemente. Layouts passen sich an, sodass bei größeren Stufen nichts abgeschnitten wird.",
    "Solid panels and brighter text and edges, for readability over busy scenes.": "Deckende Flächen sowie hellere Texte und Kanten für bessere Lesbarkeit vor unruhigen Szenen.",
    "Turns off pops, slides, pulses, screen transitions and rain streaks. Colour cues stay on.": "Schaltet Aufploppen, Gleiten, Pulse, Bildschirmübergänge und Regenschlieren ab. Farbhinweise bleiben erhalten.",
    "Hold: the gadget wheel stays open while the button is held. Toggle: press once to open and again to close.": "Halten: Das Gadget-Rad bleibt offen, solange die Taste gedrückt ist. Umschalten: Einmal drücken zum Öffnen, erneut zum Schließen.",
    "Hold: keep the button held to analyse a clue. Tap: a single press analyses it.": "Halten: Taste gedrückt halten, um einen Hinweis zu analysieren. Tippen: Ein einzelner Druck analysiert ihn.",
    "Text size for subtitles and speaker names.": "Textgröße für Untertitel und Sprechernamen.",
    "Draws a solid panel behind subtitles so they read over any scene.": "Zeichnet eine deckende Fläche hinter die Untertitel, damit sie vor jeder Szene lesbar bleiben.",
    "Change the key or button for every action, separately for keyboard and mouse and for gamepad.": "Ändere die Taste für jede Aktion, getrennt für Tastatur und Maus sowie für das Gamepad.",
    "Clue analysis": "Hinweisanalyse", "Tap": "Tippen", "Hold to analyse": "Halten zum Analysieren", "Analysing": "Analysiere", "Unknown evidence": "Unbekanntes Beweisstück", "{0} m": "{0} m",
    "Hits": "Treffer", "Objective": "Ziel", "Low": "Niedrig", "Ready": "Bereit", "{0}s": "{0} s",
    "Attack": "Angreifen", "Case file": "Fallakte", "Detective mode": "Detektivmodus",
    "Gadget 1": "Gadget 1", "Gadget 2": "Gadget 2", "Gadget 3": "Gadget 3", "Gadget wheel": "Gadget-Rad",
    "Move back": "Rückwärts", "Move forward": "Vorwärts", "Move left": "Nach links", "Move right": "Nach rechts",
    "Pause": "Pause", "Scan clue": "Hinweis scannen",
    "CASE FILE": "FALLAKTE", "Close": "Schließen", "Select": "Auswählen",
    "{0} of {1} clues discovered": "{0} von {1} Hinweisen entdeckt",
    "???": "???", "Debug clue {0}": "Debug-Hinweis {0}",
    "Generated to stress the virtualised list.": "Erzeugt, um die virtualisierte Liste zu belasten.",
    "Not yet investigated.": "Noch nicht untersucht.",
    "x{0}": "x{0}", "{0} hits": "{0} Treffer", "No": "Nein", "Yes": "Ja",
    "That button cannot be used here. Choose a gamepad button.": "Diese Taste kann hier nicht verwendet werden. Wähle eine Gamepad-Taste.",
    "That key cannot be used here. Choose another key or mouse button.": "Diese Taste kann hier nicht verwendet werden. Wähle eine andere Taste oder Maustaste.",
    "That key was already in use; the two actions swapped keys.": "Diese Taste war bereits belegt; die beiden Aktionen haben die Tasten getauscht.",
    "...": "...", "Back": "Zurück", "CONTROLS": "STEUERUNG", "Controls reset to defaults.": "Steuerung auf Standard zurückgesetzt.",
    "Gamepad": "Gamepad", "Keyboard / Mouse": "Tastatur / Maus",
    "Press a gamepad button. Choose B to cancel.": "Drücke eine Gamepad-Taste. Mit B abbrechen.",
    "Press a key or mouse button. Choose Esc to cancel.": "Drücke eine Taste oder Maustaste. Mit Esc abbrechen.",
    "Rebind": "Neu belegen", "Reset to defaults": "Auf Standard zurücksetzen", "Unbound": "Nicht belegt",
    "DETECTIVE MODE": "DETEKTIVMODUS", "Scan": "Scannen",
    "Wing-Blade": "Flügelklinge", "Grapnel": "Enterhaken", "Smoke Pellet": "Rauchkapsel",
    "LOW  {0} / {1}": "NIEDRIG  {0} / {1}", "{0} / {1}": "{0} / {1}",
    "Case solved": "Fall gelöst", "Find the clues": "Finde die Hinweise",
    "PAUSED": "PAUSIERT", "Quit": "Beenden", "Quit game?": "Spiel beenden?", "Resume": "Fortsetzen", "Settings": "Einstellungen",
    "Unsaved progress will be lost.": "Nicht gespeicherter Fortschritt geht verloren.",
    "Colour vision": "Farbsehen", "Deuteranopia": "Deuteranopie", "Protanopia": "Protanopie", "Tritanopia": "Tritanopie",
    "Hold": "Halten", "Toggle": "Umschalten", "Language": "Sprache", "Large": "Groß", "Medium": "Mittel", "Small": "Klein",
    "Off": "Aus", "On": "An", "High contrast": "Hoher Kontrast", "Reduced motion": "Reduzierte Bewegung",
    "Standard": "Standard", "Subtitle background": "Untertitelhintergrund", "Subtitle size": "Untertitelgröße",
    "UI scale": "UI-Skalierung", "{0}%": "{0} %",
    "Apply": "Übernehmen", "Back (discards unapplied changes)": "Zurück (verwirft nicht übernommene Änderungen)",
    "Controls": "Steuerung", "Defaults": "Standard", "Revert": "Zurücksetzen", "SETTINGS": "EINSTELLUNGEN",
    "Unapplied changes. Choose Apply to keep them.": "Nicht übernommene Änderungen. Wähle „Übernehmen“, um sie zu behalten.",
    "Detective": "Detektiv", "{0}. {1}": "{0}. {1}",
    "Torn ledger page": "Zerrissene Hauptbuchseite",
    "A page torn from a shipping ledger. Three crates never reached the docks.": "Eine aus einem Frachtbuch gerissene Seite. Drei Kisten haben die Docks nie erreicht.",
    "Muddy footprint": "Schlammiger Fußabdruck",
    "A size 12 boot print, heading away from the warehouse.": "Ein Stiefelabdruck Größe 46, der vom Lagerhaus wegführt.",
    "Spent shell casing": "Leere Patronenhülse",
    "9mm, fired recently. The rounding on the rim points to a custom load.": "9 mm, kürzlich abgefeuert. Die Rundung am Rand deutet auf eine Spezialladung hin.",
    "Dropped keycard": "Verlorene Schlüsselkarte",
    "Maintenance access. Someone went back for it and did not find it.": "Wartungszugang. Jemand kam zurück, um sie zu holen, und fand sie nicht.",
    "Crumpled note": "Zerknüllte Notiz",
    "Half a phone number and the word 'midnight' underlined twice.": "Eine halbe Telefonnummer und das Wort „Mitternacht“, zweimal unterstrichen.",
}

JA = {
    # V5 combat
    "Counter": "カウンター", "{0}-hit combo": "{0}ヒットコンボ",
    # V4 menus
    "Blackwater Ops": "Blackwater Ops", "Paused": "ポーズ中", "Evidence": "証拠", "Options": "オプション",
    "Investigation": "捜査", "Change": "変更", "Browse": "閲覧", "Caution": "注意",
    "Analysed": "分析済み", "Not yet found": "未発見", "No. {0}": "No. {0}", "Key bindings": "キー割り当て",
    "Display": "表示", "Accessibility": "アクセシビリティ",
    "Language for all menus, the HUD and subtitles. Changes preview immediately.": "メニュー、HUD、字幕の言語。変更はすぐにプレビューされます。",
    "Swaps the clue, danger and highlight colours for palettes that stay distinct with protanopia, deuteranopia or tritanopia.": "手がかり・危険・強調の色を、1型・2型・3型色覚でも見分けやすい配色に切り替えます。",
    "Scales every menu and HUD element. Layouts reflow, so nothing is cut off at larger sizes.": "すべてのメニューとHUDの大きさを変更します。レイアウトが再配置されるため、大きくしても切れません。",
    "Solid panels and brighter text and edges, for readability over busy scenes.": "不透明なパネルと明るい文字・縁取りで、背景が複雑でも読みやすくします。",
    "Turns off pops, slides, pulses, screen transitions and rain streaks. Colour cues stay on.": "ポップ、スライド、パルス、画面遷移、雨の筋をオフにします。色による合図は残ります。",
    "Hold: the gadget wheel stays open while the button is held. Toggle: press once to open and again to close.": "長押し：ボタンを押している間ガジェットホイールを表示します。切り替え：一度押すと開き、もう一度押すと閉じます。",
    "Hold: keep the button held to analyse a clue. Tap: a single press analyses it.": "長押し：ボタンを押し続けて手がかりを分析します。タップ：一度押すだけで分析します。",
    "Text size for subtitles and speaker names.": "字幕と話者名の文字サイズ。",
    "Draws a solid panel behind subtitles so they read over any scene.": "字幕の背後に不透明なパネルを表示し、どの場面でも読みやすくします。",
    "Change the key or button for every action, separately for keyboard and mouse and for gamepad.": "すべての操作のキーやボタンを、キーボード・マウスとゲームパッドで個別に変更できます。",
    "Clue analysis": "手がかり分析", "Tap": "タップ", "Hold to analyse": "長押しで分析", "Analysing": "分析中", "Unknown evidence": "不明な証拠", "{0} m": "{0} m",
    "Hits": "ヒット", "Objective": "目標", "Low": "低下", "Ready": "準備完了", "{0}s": "{0}秒",
    "Attack": "攻撃", "Case file": "事件ファイル", "Detective mode": "探偵モード",
    "Gadget 1": "ガジェット 1", "Gadget 2": "ガジェット 2", "Gadget 3": "ガジェット 3", "Gadget wheel": "ガジェットホイール",
    "Move back": "後退", "Move forward": "前進", "Move left": "左へ移動", "Move right": "右へ移動",
    "Pause": "ポーズ", "Scan clue": "手がかりをスキャン",
    "CASE FILE": "事件ファイル", "Close": "閉じる", "Select": "選択",
    "{0} of {1} clues discovered": "{1}件中{0}件の手がかりを発見",
    "???": "???", "Debug clue {0}": "デバッグ手がかり {0}",
    "Generated to stress the virtualised list.": "仮想化リストの負荷テスト用に生成されました。",
    "Not yet investigated.": "未調査です。",
    "x{0}": "x{0}", "{0} hits": "{0}ヒット", "No": "いいえ", "Yes": "はい",
    "That button cannot be used here. Choose a gamepad button.": "このボタンはここでは使用できません。ゲームパッドのボタンを選んでください。",
    "That key cannot be used here. Choose another key or mouse button.": "このキーはここでは使用できません。別のキーまたはマウスボタンを選んでください。",
    "That key was already in use; the two actions swapped keys.": "そのキーはすでに使用されていたため、2つの操作のキーを入れ替えました。",
    "...": "...", "Back": "戻る", "CONTROLS": "操作設定", "Controls reset to defaults.": "操作設定を初期値に戻しました。",
    "Gamepad": "ゲームパッド", "Keyboard / Mouse": "キーボード / マウス",
    "Press a gamepad button. Choose B to cancel.": "ゲームパッドのボタンを押してください。Bでキャンセル。",
    "Press a key or mouse button. Choose Esc to cancel.": "キーまたはマウスボタンを押してください。Escでキャンセル。",
    "Rebind": "再割り当て", "Reset to defaults": "初期設定に戻す", "Unbound": "未割り当て",
    "DETECTIVE MODE": "探偵モード", "Scan": "スキャン",
    "Wing-Blade": "ウイングブレード", "Grapnel": "グラップル", "Smoke Pellet": "煙幕弾",
    "LOW  {0} / {1}": "低下  {0} / {1}", "{0} / {1}": "{0} / {1}",
    "Case solved": "事件解決", "Find the clues": "手がかりを探せ",
    "PAUSED": "ポーズ中", "Quit": "終了", "Quit game?": "ゲームを終了しますか？", "Resume": "再開", "Settings": "設定",
    "Unsaved progress will be lost.": "保存されていない進行状況は失われます。",
    "Colour vision": "色覚", "Deuteranopia": "緑色覚異常(D型)", "Protanopia": "赤色覚異常(P型)", "Tritanopia": "青色覚異常(T型)",
    "Hold": "長押し", "Toggle": "切り替え", "Language": "言語", "Large": "大", "Medium": "中", "Small": "小",
    "Off": "オフ", "On": "オン", "High contrast": "ハイコントラスト", "Reduced motion": "モーション軽減",
    "Standard": "標準", "Subtitle background": "字幕の背景", "Subtitle size": "字幕サイズ",
    "UI scale": "UIスケール", "{0}%": "{0}%",
    "Apply": "適用", "Back (discards unapplied changes)": "戻る（未適用の変更は破棄されます）",
    "Controls": "操作", "Defaults": "初期値", "Revert": "元に戻す", "SETTINGS": "設定",
    "Unapplied changes. Choose Apply to keep them.": "未適用の変更があります。保持するには「適用」を選んでください。",
    "Detective": "探偵", "{0}. {1}": "{0}。{1}",
    "Torn ledger page": "破れた台帳のページ",
    "A page torn from a shipping ledger. Three crates never reached the docks.": "船荷台帳から破り取られたページ。3つの木箱は埠頭に届かなかった。",
    "Muddy footprint": "泥だらけの足跡",
    "A size 12 boot print, heading away from the warehouse.": "倉庫から離れていく、サイズ12のブーツの足跡。",
    "Spent shell casing": "使用済みの薬莢",
    "9mm, fired recently. The rounding on the rim points to a custom load.": "9mm、最近発射されたもの。縁の丸みから、特注の装填だと分かる。",
    "Dropped keycard": "落とされたキーカード",
    "Maintenance access. Someone went back for it and did not find it.": "整備用アクセス権。誰かが取りに戻ったが、見つけられなかった。",
    "Crumpled note": "くしゃくしゃのメモ",
    "Half a phone number and the word 'midnight' underlined twice.": "電話番号の半分と、二重に下線が引かれた「真夜中」の文字。",
}

ACCENTS = str.maketrans("aeiouAEIOUcnysCNYS", "áéíóúÅÉÍÓÚçñýšÇÑÝŠ")


def pseudo(text):
    """Accented, ~40% longer, bracketed. {n} placeholders pass through untouched."""
    parts = re.split(r"(\{\d+\})", text)
    body = "".join(p if re.fullmatch(r"\{\d+\}", p) else p.translate(ACCENTS) for p in parts)
    padding = "~" * max(2, int(len(re.sub(r"\{\d+\}", "", text)) * 0.4))
    return f"[{body} {padding}]"


def manifest_sources():
    """(namespace path, key) -> current source text, from the gathered manifest."""
    manifest = json.load(open(f"{ROOT}\\Game.manifest", encoding="utf-16"))
    sources = {}

    def walk(node, path):
        path = path + (node.get("Namespace", ""),)
        for child in node.get("Children", []):
            for key in child.get("Keys", []):
                sources[(path, key["Key"])] = child["Source"]["Text"]
        for sub in node.get("Subnamespaces", []):
            walk(sub, path)

    walk(manifest, ())
    return sources


SOURCES = manifest_sources()


def fill(culture, table, make=None):
    path = f"{ROOT}\\{culture}\\Game.archive"
    archive = json.load(open(path, encoding="utf-16"))
    missing = []

    def walk(node, ns_path=()):
        ns_path = ns_path + (node.get("Namespace", ""),)
        for child in node.get("Children", []):
            # GatherText keeps an archive entry's old source when only the source text changed (same key), so the
            # translation would silently stay attached to the old text. Re-point it at the manifest's source.
            current = SOURCES.get((ns_path, child.get("Key")))
            if current is not None and current != child["Source"]["Text"]:
                child["Source"]["Text"] = current
            source = child["Source"]["Text"]
            if make:
                child["Translation"]["Text"] = make(source)
            elif source in table:
                child["Translation"]["Text"] = table[source]
            else:
                missing.append(source)
        for sub in node.get("Subnamespaces", []):
            walk(sub, ns_path)

    walk(archive)
    json.dump(archive, open(path, "w", encoding="utf-16"), ensure_ascii=False, indent="\t")
    print(f"{culture}: filled, {len(missing)} missing")
    for text in missing:
        print("   missing:", text)


fill("de", DE)
fill("ja", JA)
fill("en-XA", None, make=pseudo)
sys.exit(0)
