#if os(macOS)
import Cocoa

let isEnglish = (ProcessInfo.processInfo.environment["PROGVORAZ_LANG"] ?? "").lowercased().hasPrefix("en")

func tr(_ es: String, _ en: String) -> String {
    return isEnglish ? en : es
}

func normalize(_ s: String) -> String {
    let trimmed = s.trimmingCharacters(in: .whitespacesAndNewlines)
    let clean = trimmed.isEmpty ? "0" : trimmed
    let dropped = clean.drop(while: { $0 == "0" })
    return dropped.isEmpty ? "0" : String(dropped)
}

func isDigits(_ s: String) -> Bool {
    if s.isEmpty { return false }
    return s.allSatisfy { $0 >= "0" && $0 <= "9" }
}

func isIntegerToken(_ s: String) -> Bool {
    if s.isEmpty { return false }
    if s.first == "-" { return s.dropFirst().allSatisfy { $0 >= "0" && $0 <= "9" } }
    return isDigits(s)
}

func compareBig(_ a: String, _ b: String) -> Int {
    let x = normalize(a)
    let y = normalize(b)
    if x.count != y.count { return x.count < y.count ? -1 : 1 }
    if x == y { return 0 }
    return x < y ? -1 : 1
}

func addBig(_ a: String, _ b: String) -> String {
    let x = Array(normalize(a))
    let y = Array(normalize(b))
    var i = x.count - 1
    var j = y.count - 1
    var carry = 0
    var out: [Character] = []

    while i >= 0 || j >= 0 || carry > 0 {
        let da = i >= 0 ? Int(String(x[i]))! : 0
        let db = j >= 0 ? Int(String(y[j]))! : 0
        let s = da + db + carry
        out.append(Character(String(s % 10)))
        carry = s / 10
        i -= 1
        j -= 1
    }

    return String(out.reversed())
}

func subBig(_ a: String, _ b: String) -> String? {
    if compareBig(a, b) < 0 { return nil }

    let x = Array(normalize(a))
    let y = Array(normalize(b))
    var i = x.count - 1
    var j = y.count - 1
    var borrow = 0
    var out: [Character] = []

    while i >= 0 {
        var da = Int(String(x[i]))! - borrow
        let db = j >= 0 ? Int(String(y[j]))! : 0
        if da < db {
            da += 10
            borrow = 1
        } else {
            borrow = 0
        }

        out.append(Character(String(da - db)))
        i -= 1
        j -= 1
    }

    let res = String(out.reversed())
    return normalize(res)
}

func mulBig(_ a: String, _ b: String) -> String {
    let x = Array(normalize(a).reversed())
    let y = Array(normalize(b).reversed())

    if x.count == 1 && x[0] == "0" { return "0" }
    if y.count == 1 && y[0] == "0" { return "0" }

    var out = Array(repeating: 0, count: x.count + y.count)

    for i in 0..<x.count {
        let da = Int(String(x[i]))!
        for j in 0..<y.count {
            let db = Int(String(y[j]))!
            out[i + j] += da * db
        }
    }

    for k in 0..<(out.count - 1) {
        if out[k] >= 10 {
            out[k + 1] += out[k] / 10
            out[k] %= 10
        }
    }

    while out.count > 1 && out.last == 0 {
        out.removeLast()
    }

    return out.reversed().map(String.init).joined()
}

func divmodBig(_ a: String, _ b: String) -> (q: String, r: String)? {
    let dividend = normalize(a)
    let divisor = normalize(b)

    if divisor == "0" { return nil }
    if compareBig(dividend, divisor) < 0 { return ("0", dividend) }

    var qChars: [Character] = []
    var current = "0"

    for ch in dividend {
        current = addBig(mulBig(current, "10"), String(ch))

        var digit = 0
        for d in stride(from: 9, through: 0, by: -1) {
            let prod = mulBig(divisor, String(d))
            if compareBig(prod, current) <= 0 {
                digit = d
                current = subBig(current, prod)!
                break
            }
        }

        qChars.append(Character(String(digit)))
    }

    return (normalize(String(qChars)), normalize(current))
}

func readTokens(from path: String) -> [String]? {
    guard let txt = try? String(contentsOfFile: path, encoding: .utf8) else { return nil }
    return txt.split { $0.isWhitespace }.map(String.init)
}

func writeTokens(_ tokens: [String], to path: String) -> Bool {
    let text = tokens.joined(separator: "\n") + "\n"
    do {
        try text.write(toFile: path, atomically: true, encoding: .utf8)
        return true
    } catch {
        return false
    }
}

func loadCoinNames() -> [String] {
    guard let tokens = readTokens(from: "monedas.txt") else { return [] }
    var names: [String] = []
    for t in tokens where !isIntegerToken(t) {
        if !names.contains(t) { names.append(t) }
    }
    return names
}

func loadSection(file: String, coin: String, minusOneAsZero: Bool) -> [String]? {
    guard let tokens = readTokens(from: file) else { return nil }
    guard let idx = tokens.firstIndex(of: coin) else { return nil }

    var out: [String] = []
    var i = idx + 1
    while i < tokens.count {
        var t = tokens[i]
        if !isIntegerToken(t) { break }
        if minusOneAsZero && t == "-1" { t = "0" }
        if !isDigits(t) { break }
        out.append(normalize(t))
        i += 1
    }

    return out.isEmpty ? nil : out
}

func updateStockSection(coin: String, stock: [String]) -> Bool {
    guard var tokens = readTokens(from: "stock.txt") else { return false }
    guard let idx = tokens.firstIndex(of: coin) else { return false }

    var i = idx + 1
    for s in stock {
        if i >= tokens.count || !isIntegerToken(tokens[i]) { return false }
        tokens[i] = normalize(s) == "0" ? "-1" : normalize(s)
        i += 1
    }

    return writeTokens(tokens, to: "stock.txt")
}

func registerHistory(_ msg: String) {
    let formatter = DateFormatter()
    formatter.dateFormat = "yyyy-MM-dd HH:mm:ss"
    let dateString = formatter.string(from: Date())
    let text = "[\(dateString)] \(msg)\n"
    let file = "historial.txt"
    if let handle = try? FileHandle(forWritingTo: URL(fileURLWithPath: file)) {
        handle.seekToEndOfFile()
        if let data = text.data(using: .utf8) { handle.write(data) }
        handle.closeFile()
    } else {
        try? text.write(toFile: file, atomically: true, encoding: .utf8)
    }
}

func createStockSnapshot(snapshotPath: String = "stock_snapshot.txt") -> Bool {
    let fm = FileManager.default
    if fm.fileExists(atPath: snapshotPath) {
        try? fm.removeItem(atPath: snapshotPath)
    }
    do {
        try fm.copyItem(atPath: "stock.txt", toPath: snapshotPath)
        return true
    } catch {
        return false
    }
}

func restoreStockSnapshot(snapshotPath: String = "stock_snapshot.txt") -> Bool {
    let fm = FileManager.default
    guard fm.fileExists(atPath: snapshotPath) else { return false }
    do {
        if fm.fileExists(atPath: "stock.txt") {
            try fm.removeItem(atPath: "stock.txt")
        }
        try fm.copyItem(atPath: snapshotPath, toPath: "stock.txt")
        return true
    } catch {
        return false
    }
}

func runProgvorazCLI(arguments: [String]) -> (ok: Bool, output: String) {
    let process = Process()
    let pipe = Pipe()

    process.currentDirectoryURL = URL(fileURLWithPath: FileManager.default.currentDirectoryPath)
    process.executableURL = URL(fileURLWithPath: FileManager.default.currentDirectoryPath + "/progvoraz")
    process.arguments = arguments
    process.standardOutput = pipe
    process.standardError = pipe

    do {
        try process.run()
    } catch {
        return (false, "\(tr("No se pudo ejecutar progvoraz", "Could not execute progvoraz")): \(error.localizedDescription)")
    }

    process.waitUntilExit()
    let data = pipe.fileHandleForReading.readDataToEndOfFile()
    let output = String(data: data, encoding: .utf8) ?? ""
    return (process.terminationStatus == 0, output.trimmingCharacters(in: .whitespacesAndNewlines))
}

final class AppDelegate: NSObject, NSApplicationDelegate, NSTableViewDataSource {
    var window: NSWindow!
    var coinPopup: NSPopUpButton!
    var denomPopup: NSPopUpButton!
    var qtyField: NSTextField!
    var amountField: NSTextField!
    var registerCheck: NSButton!
    var priceField: NSTextField!
    var paymentField: NSTextField!
    var limitField: NSTextField!
    var historyBtn: NSButton!
    var summaryBtn: NSButton!
    var originField: NSTextField!
    var convertAmountField: NSTextField!
    var convertUseStockCheck: NSButton!
    var table: NSTableView!
    var resultView: NSTextView!
    var statusLabel: NSTextField!
    var modeLimitedBtn: NSButton!
    var modeUnlimitedBtn: NSButton!
    var addBtn: NSButton!
    var subBtn: NSButton!
    var specificInField: NSTextField!
    var specificOutField: NSTextField!
    var specificBtn: NSButton!

    var coinNames: [String] = []
    var activeCoin: String?
    var denoms: [String] = []
    var stock: [String] = []
    var limitedMode = true

    func applicationDidFinishLaunching(_ notification: Notification) {
        setupUI()
        reloadCoins()
    }

    func setupUI() {
        window = NSWindow(
            contentRect: NSRect(x: 0, y: 0, width: 780, height: 760),
            styleMask: [.titled, .closable, .miniaturizable],
            backing: .buffered,
            defer: false
        )
        window.title = tr("ProgVoraz GUI macOS (Swift)", "ProgVoraz macOS GUI (Swift)")
        window.center()

        let content = window.contentView!

        let title = NSTextField(labelWithString: tr("Panel GUI (macOS) - Modos", "GUI Panel (macOS) - Modes"))
        title.frame = NSRect(x: 20, y: 600, width: 360, height: 24)
        title.font = NSFont.boldSystemFont(ofSize: 16)
        content.addSubview(title)

        let coinLabel = NSTextField(labelWithString: tr("Moneda", "Currency"))
        coinLabel.frame = NSRect(x: 20, y: 568, width: 60, height: 22)
        content.addSubview(coinLabel)

        coinPopup = NSPopUpButton(frame: NSRect(x: 85, y: 565, width: 180, height: 26), pullsDown: false)
        content.addSubview(coinPopup)

        let loadBtn = NSButton(title: tr("Cargar", "Load"), target: self, action: #selector(loadCoin))
        loadBtn.frame = NSRect(x: 275, y: 564, width: 90, height: 28)
        content.addSubview(loadBtn)

        let reloadBtn = NSButton(title: tr("Recargar", "Reload"), target: self, action: #selector(reloadCoinsAction))
        reloadBtn.frame = NSRect(x: 375, y: 564, width: 90, height: 28)
        content.addSubview(reloadBtn)

        modeLimitedBtn = NSButton(radioButtonWithTitle: tr("Stock limitado", "Limited stock"), target: self, action: #selector(modeChanged(_:)))
        modeLimitedBtn.frame = NSRect(x: 20, y: 532, width: 140, height: 22)
        modeLimitedBtn.state = .on
        content.addSubview(modeLimitedBtn)

        modeUnlimitedBtn = NSButton(radioButtonWithTitle: tr("Stock ilimitado", "Unlimited stock"), target: self, action: #selector(modeChanged(_:)))
        modeUnlimitedBtn.frame = NSRect(x: 170, y: 532, width: 150, height: 22)
        content.addSubview(modeUnlimitedBtn)

        let scroll = NSScrollView(frame: NSRect(x: 20, y: 290, width: 740, height: 230))
        table = NSTableView(frame: scroll.bounds)

        let col1 = NSTableColumn(identifier: NSUserInterfaceItemIdentifier("denom"))
        col1.title = tr("Denominacion", "Denomination")
        col1.width = 300

        let col2 = NSTableColumn(identifier: NSUserInterfaceItemIdentifier("stock"))
        col2.title = tr("Stock", "Stock")
        col2.width = 420

        table.addTableColumn(col1)
        table.addTableColumn(col2)
        table.dataSource = self

        scroll.documentView = table
        scroll.hasVerticalScroller = true
        content.addSubview(scroll)

        let denomLabel = NSTextField(labelWithString: tr("Denominacion", "Denomination"))
        denomLabel.frame = NSRect(x: 20, y: 250, width: 90, height: 22)
        content.addSubview(denomLabel)

        denomPopup = NSPopUpButton(frame: NSRect(x: 115, y: 247, width: 180, height: 26), pullsDown: false)
        content.addSubview(denomPopup)

        let qtyLabel = NSTextField(labelWithString: tr("Cantidad", "Quantity"))
        qtyLabel.frame = NSRect(x: 310, y: 250, width: 70, height: 22)
        content.addSubview(qtyLabel)

        qtyField = NSTextField(frame: NSRect(x: 385, y: 246, width: 160, height: 26))
        content.addSubview(qtyField)

        addBtn = NSButton(title: tr("Anadir", "Add"), target: self, action: #selector(addStock))
        addBtn.frame = NSRect(x: 230, y: 204, width: 100, height: 30)
        content.addSubview(addBtn)

        subBtn = NSButton(title: tr("Quitar", "Remove"), target: self, action: #selector(subStock))
        subBtn.frame = NSRect(x: 340, y: 204, width: 100, height: 30)
        content.addSubview(subBtn)

        let specificTitle = NSTextField(labelWithString: tr("Cambio especifico (cantidades separadas por espacios, en orden de denominaciones)", "Specific change (space-separated quantities, in denomination order)"))
        specificTitle.frame = NSRect(x: 20, y: 168, width: 720, height: 22)
        content.addSubview(specificTitle)

        let specificInLabel = NSTextField(labelWithString: tr("Entregado", "Delivered"))
        specificInLabel.frame = NSRect(x: 20, y: 142, width: 80, height: 22)
        content.addSubview(specificInLabel)

        specificInField = NSTextField(frame: NSRect(x: 105, y: 138, width: 560, height: 26))
        content.addSubview(specificInField)

        let specificOutLabel = NSTextField(labelWithString: tr("Devolucion", "Return"))
        specificOutLabel.frame = NSRect(x: 20, y: 112, width: 80, height: 22)
        content.addSubview(specificOutLabel)

        specificOutField = NSTextField(frame: NSRect(x: 105, y: 108, width: 560, height: 26))
        content.addSubview(specificOutField)

        specificBtn = NSButton(title: tr("Aplicar cambio especifico", "Apply specific change"), target: self, action: #selector(applySpecificChange))
        specificBtn.frame = NSRect(x: 670, y: 108, width: 90, height: 56)
        content.addSubview(specificBtn)

        registerCheck = NSButton(checkboxWithTitle: tr("Caja Reg.", "Register"), target: self, action: #selector(toggleRegister))
        registerCheck.frame = NSRect(x: 20, y: 70, width: 90, height: 22)
        content.addSubview(registerCheck)

        let priceLabel = NSTextField(labelWithString: tr("Precio:", "Price:"))
        priceLabel.frame = NSRect(x: 115, y: 70, width: 45, height: 22)
        content.addSubview(priceLabel)

        priceField = NSTextField(frame: NSRect(x: 160, y: 66, width: 60, height: 26))
        priceField.isEnabled = false
        content.addSubview(priceField)

        let paymentLabel = NSTextField(labelWithString: tr("Pago:", "Payment:"))
        paymentLabel.frame = NSRect(x: 225, y: 70, width: 40, height: 22)
        content.addSubview(paymentLabel)

        paymentField = NSTextField(frame: NSRect(x: 265, y: 66, width: 60, height: 26))
        paymentField.isEnabled = false
        content.addSubview(paymentField)

        let amountLabel = NSTextField(labelWithString: tr("Monto:", "Amount:"))
        amountLabel.frame = NSRect(x: 330, y: 70, width: 50, height: 22)
        content.addSubview(amountLabel)

        amountField = NSTextField(frame: NSRect(x: 380, y: 66, width: 60, height: 26))
        content.addSubview(amountField)

        let limitLabel = NSTextField(labelWithString: tr("Limite:", "Limit:"))
        limitLabel.frame = NSRect(x: 445, y: 70, width: 45, height: 22)
        content.addSubview(limitLabel)

        limitField = NSTextField(frame: NSRect(x: 490, y: 66, width: 50, height: 26))
        content.addSubview(limitField)

        let calcBtn = NSButton(title: tr("Calcular", "Calculate"), target: self, action: #selector(calculateChange))
        calcBtn.frame = NSRect(x: 550, y: 64, width: 80, height: 30)
        content.addSubview(calcBtn)

        historyBtn = NSButton(title: tr("Historial", "History"), target: self, action: #selector(showHistory))
        historyBtn.frame = NSRect(x: 640, y: 64, width: 80, height: 30)
        content.addSubview(historyBtn)

        summaryBtn = NSButton(title: tr("Resumen", "Summary"), target: self, action: #selector(showSummary))
        summaryBtn.frame = NSRect(x: 550, y: 34, width: 80, height: 26)
        content.addSubview(summaryBtn)

        let snapshotBtn = NSButton(title: "Snapshot", target: self, action: #selector(createSnapshot))
        snapshotBtn.frame = NSRect(x: 20, y: 34, width: 90, height: 26)
        content.addSubview(snapshotBtn)

        let restoreBtn = NSButton(title: tr("Restaurar", "Restore"), target: self, action: #selector(restoreSnapshot))
        restoreBtn.frame = NSRect(x: 120, y: 34, width: 90, height: 26)
        content.addSubview(restoreBtn)

        let reportBtn = NSButton(title: tr("Reporte", "Report"), target: self, action: #selector(exportReport))
        reportBtn.frame = NSRect(x: 220, y: 34, width: 80, height: 26)
        content.addSubview(reportBtn)

        let jsonBtn = NSButton(title: "JSON", target: self, action: #selector(exportStockJSON))
        jsonBtn.frame = NSRect(x: 310, y: 34, width: 70, height: 26)
        content.addSubview(jsonBtn)

        let convertBtn = NSButton(title: tr("Convertir", "Convert"), target: self, action: #selector(convertCurrency))
        convertBtn.frame = NSRect(x: 390, y: 34, width: 80, height: 26)
        content.addSubview(convertBtn)

        let resultTitle = NSTextField(labelWithString: tr("Resultado", "Result"))
        resultTitle.frame = NSRect(x: 20, y: 42, width: 120, height: 22)
        content.addSubview(resultTitle)

        let resultScroll = NSScrollView(frame: NSRect(x: 20, y: 44, width: 740, height: 90))
        resultView = NSTextView(frame: resultScroll.bounds)
        resultView.isEditable = false
        resultView.font = NSFont.monospacedSystemFont(ofSize: 12, weight: .regular)
        resultScroll.documentView = resultView
        resultScroll.hasVerticalScroller = true
        content.addSubview(resultScroll)

        let originLabel = NSTextField(labelWithString: tr("Origen", "Origin"))
        originLabel.frame = NSRect(x: 20, y: 8, width: 50, height: 22)
        content.addSubview(originLabel)

        originField = NSTextField(frame: NSRect(x: 70, y: 6, width: 110, height: 24))
        content.addSubview(originField)

        let convertAmountLabel = NSTextField(labelWithString: tr("Monto c", "Amount c"))
        convertAmountLabel.frame = NSRect(x: 190, y: 8, width: 55, height: 22)
        content.addSubview(convertAmountLabel)

        convertAmountField = NSTextField(frame: NSRect(x: 245, y: 6, width: 110, height: 24))
        content.addSubview(convertAmountField)

        convertUseStockCheck = NSButton(checkboxWithTitle: tr("Usar stock destino", "Use destination stock"), target: nil, action: nil)
        convertUseStockCheck.frame = NSRect(x: 365, y: 6, width: 150, height: 24)
        content.addSubview(convertUseStockCheck)

        statusLabel = NSTextField(labelWithString: "")
        statusLabel.frame = NSRect(x: 525, y: 8, width: 235, height: 24)
        content.addSubview(statusLabel)

        window.makeKeyAndOrderFront(nil)
        applyModeToUI()
    }

    func numberOfRows(in tableView: NSTableView) -> Int {
        return denoms.count
    }

    func tableView(_ tableView: NSTableView, objectValueFor tableColumn: NSTableColumn?, row: Int) -> Any? {
        if row < 0 || row >= denoms.count { return nil }
        if tableColumn?.identifier.rawValue == "denom" {
            return denoms[row] + " c"
        }
        return limitedMode ? stock[row] : tr("Ilimitado", "Unlimited")
    }

    func setStatus(_ text: String, error: Bool = false) {
        statusLabel.stringValue = text
        statusLabel.textColor = error ? .systemRed : .labelColor
    }

    func reloadCoins() {
        coinNames = loadCoinNames()
        coinPopup.removeAllItems()
        coinPopup.addItems(withTitles: coinNames)
        denoms = []
        stock = []
        denomPopup.removeAllItems()
        table.reloadData()
        resultView.string = ""
        specificInField?.stringValue = ""
        specificOutField?.stringValue = ""

        if coinNames.isEmpty {
            setStatus(tr("No se pudieron leer monedas desde monedas.txt", "Could not read currencies from monedas.txt"), error: true)
        } else {
            setStatus(tr("Monedas cargadas. Selecciona una y pulsa Cargar.", "Currencies loaded. Select one and press Load."))
        }
    }

    @objc func reloadCoinsAction() {
        reloadCoins()
    }

    func applyModeToUI() {
        addBtn.isEnabled = limitedMode
        subBtn.isEnabled = limitedMode
        denomPopup.isEnabled = limitedMode
        qtyField.isEnabled = limitedMode
        specificInField.isEnabled = true
        specificOutField.isEnabled = true
        specificBtn.isEnabled = true
        table.reloadData()
    }

    @objc func modeChanged(_ sender: NSButton) {
        limitedMode = (sender == modeLimitedBtn)
        modeLimitedBtn.state = limitedMode ? .on : .off
        modeUnlimitedBtn.state = limitedMode ? .off : .on
        resultView.string = ""
        applyModeToUI()
        setStatus(limitedMode ? tr("Modo stock limitado activo.", "Limited stock mode active.") : tr("Modo stock ilimitado activo.", "Unlimited stock mode active."))
    }

    @objc func loadCoin() {
        guard let coin = coinPopup.titleOfSelectedItem, !coin.isEmpty else {
            setStatus(tr("Selecciona una moneda valida.", "Select a valid currency."), error: true)
            return
        }

        guard let d = loadSection(file: "monedas.txt", coin: coin, minusOneAsZero: false),
              let s = loadSection(file: "stock.txt", coin: coin, minusOneAsZero: true),
              d.count == s.count else {
            setStatus(tr("No se pudo cargar denominaciones/stock.", "Could not load denominations/stock."), error: true)
            return
        }

        activeCoin = coin
        denoms = d
        stock = s
        denomPopup.removeAllItems()
        denomPopup.addItems(withTitles: denoms)
        table.reloadData()
        resultView.string = ""
        let zeros = Array(repeating: "0", count: denoms.count).joined(separator: " ")
        specificInField.stringValue = zeros
        specificOutField.stringValue = zeros
        setStatus("\(tr("Moneda", "Currency")) \(coin) \(tr("cargada.", "loaded."))")
    }

    func parseSpecificList(_ text: String, expected: Int) -> [String]? {
        let tokens = text.split { $0.isWhitespace }.map(String.init)
        if tokens.count != expected { return nil }
        if tokens.contains(where: { !isDigits(normalize($0)) }) { return nil }
        return tokens.map { normalize($0) }
    }

    func totalValue(_ quantities: [String]) -> String {
        var total = "0"
        for i in 0..<min(denoms.count, quantities.count) {
            if normalize(quantities[i]) == "0" { continue }
            total = addBig(total, mulBig(denoms[i], quantities[i]))
        }
        return normalize(total)
    }

    func renderSpecificResult(total: String, requested: [String], unlimited: Bool) {
        var lines: [String] = []
            lines.append(unlimited ? "\(tr("Cambio especifico aplicado (ilimitado):", "Specific change applied (unlimited):")) \(total) c" : "\(tr("Cambio especifico aplicado (limitado):", "Specific change applied (limited):")) \(total) c")
        var hasItems = false
        for i in 0..<requested.count {
            if normalize(requested[i]) == "0" { continue }
            lines.append("\(denoms[i]) c -> \(requested[i])")
            hasItems = true
        }
        if !hasItems {
            lines.append(tr("No se requieren monedas para devolver 0.", "No coins are required to return 0."))
        }
        resultView.string = lines.joined(separator: "\n")
    }

    @objc func applySpecificChange() {
        guard let coin = activeCoin else {
            setStatus(tr("Primero carga una moneda.", "Load a currency first."), error: true)
            return
        }

        guard let given = parseSpecificList(specificInField.stringValue, expected: denoms.count),
              let requested = parseSpecificList(specificOutField.stringValue, expected: denoms.count) else {
            setStatus(tr("Formato invalido: usa exactamente una cantidad por denominacion en ambos campos.", "Invalid format: use exactly one quantity per denomination in both fields."), error: true)
            return
        }

        let totalGiven = totalValue(given)
        let totalRequested = totalValue(requested)
        if compareBig(totalGiven, totalRequested) != 0 {
            setStatus("\(tr("Total entregado", "Delivered total")) (\(totalGiven) c) \(tr("y total solicitado", "and requested total")) (\(totalRequested) c) \(tr("deben ser iguales.", "must match."))", error: true)
            return
        }

        if !limitedMode {
            renderSpecificResult(total: totalGiven, requested: requested, unlimited: true)
            setStatus(tr("Cambio especifico aplicado en modo ilimitado.", "Specific change applied in unlimited mode."))
            registerHistory("Cambio especifico aplicado (ilimitado, macOS).")
            return
        }

        var newStock = stock
        for i in 0..<newStock.count {
            let afterIn = addBig(newStock[i], given[i])
            guard let afterOut = subBig(afterIn, requested[i]) else {
            setStatus(tr("No se puede aplicar la devolucion especifica con el stock disponible.", "Cannot apply the specific return with the available stock."), error: true)
                return
            }
            newStock[i] = afterOut
        }

        if !updateStockSection(coin: coin, stock: newStock) {
            setStatus(tr("No se pudo persistir el cambio especifico en stock.txt", "Could not persist the specific change to stock.txt"), error: true)
            return
        }

        stock = newStock
        table.reloadData()
        renderSpecificResult(total: totalGiven, requested: requested, unlimited: false)
        setStatus(tr("Cambio especifico aplicado y stock persistido.", "Specific change applied and stock persisted."))
        registerHistory("Cambio especifico aplicado con stock (macOS).")
    }

    func applyChange(isAdd: Bool) {
        if !limitedMode {
            setStatus(tr("Panel administrador disponible solo en modo stock limitado.", "Administrator panel is only available in limited stock mode."), error: true)
            return
        }

        guard let coin = activeCoin else {
            setStatus("Primero carga una moneda.", error: true)
            return
        }

        let idx = denomPopup.indexOfSelectedItem
        if idx < 0 || idx >= denoms.count {
            setStatus(tr("Selecciona una denominacion valida.", "Select a valid denomination."), error: true)
            return
        }

        let qty = normalize(qtyField.stringValue)
        if !isDigits(qty) {
            setStatus(tr("Cantidad invalida: usa entero no negativo.", "Invalid quantity: use a non-negative integer."), error: true)
            return
        }

        let old = stock[idx]
        if isAdd {
            stock[idx] = addBig(stock[idx], qty)
        } else {
            guard let v = subBig(stock[idx], qty) else {
                setStatus(tr("No puedes quitar mas stock del disponible.", "You cannot remove more stock than is available."), error: true)
                return
            }
            stock[idx] = v
        }

        if !updateStockSection(coin: coin, stock: stock) {
            stock[idx] = old
            setStatus(tr("No se pudo persistir stock en stock.txt", "Could not persist stock to stock.txt"), error: true)
            return
        }

        qtyField.stringValue = ""
        table.reloadData()
        registerHistory("Admin \(isAdd ? "ANADIR" : "QUITAR") (macOS) | Moneda=\(coin) | Denom=\(denoms[idx]) c | Cantidad=\(qty)")
        setStatus(isAdd ? tr("Stock actualizado (suma).", "Stock updated (add).") : tr("Stock actualizado (resta).", "Stock updated (remove)."))
    }

    @objc func toggleRegister() {
        let isReg = registerCheck.state == .on
        priceField.isEnabled = isReg
        paymentField.isEnabled = isReg
        amountField.isEnabled = !isReg
    }

    @objc func showHistory() {
        let path = FileManager.default.currentDirectoryPath + "/historial.txt"
        if !FileManager.default.fileExists(atPath: path) {
            FileManager.default.createFile(atPath: path, contents: Data(), attributes: nil)
        }
        NSWorkspace.shared.openFile(path)
    }

    @objc func showSummary() {
        guard let coin = activeCoin, !denoms.isEmpty else {
            setStatus("Primero carga una moneda.", error: true)
            return
        }

        var minDenom = denoms[0]
        var maxDenom = denoms[0]
        for d in denoms {
            if compareBig(d, minDenom) < 0 { minDenom = d }
            if compareBig(d, maxDenom) > 0 { maxDenom = d }
        }

        var lines: [String] = []
        lines.append("Moneda: \(coin)")
        lines.append("Denominaciones: \(denoms.count)")
        lines.append("Min: \(minDenom) c")
        lines.append("Max: \(maxDenom) c")

        if !limitedMode {
            lines.append("Modo stock ilimitado: no se calcula inventario fisico.")
            resultView.string = lines.joined(separator: "\n")
            setStatus("Resumen mostrado (modo ilimitado).")
            registerHistory("Resumen consultado (macOS) | Moneda=\(coin) | Modo=Ilimitado")
            return
        }

        var totalUnits = "0"
        var totalValueStock = "0"
        for i in 0..<min(denoms.count, stock.count) {
            totalUnits = addBig(totalUnits, stock[i])
            totalValueStock = addBig(totalValueStock, mulBig(denoms[i], stock[i]))
        }

        lines.append("Piezas en stock: \(normalize(totalUnits))")
        lines.append("Valor total stock: \(normalize(totalValueStock)) c")
        resultView.string = lines.joined(separator: "\n")
        setStatus("Resumen mostrado (modo limitado).")
        registerHistory("Resumen consultado (macOS) | Moneda=\(coin) | Piezas=\(normalize(totalUnits)) | Valor=\(normalize(totalValueStock)) c")
    }

    @objc func createSnapshot() {
        if createStockSnapshot() {
            setStatus("Snapshot creado: stock_snapshot.txt")
            registerHistory("Snapshot de stock creado (macOS).")
        } else {
            setStatus("No se pudo crear stock_snapshot.txt", error: true)
        }
    }

    @objc func restoreSnapshot() {
        if !restoreStockSnapshot() {
            setStatus("No se pudo restaurar stock_snapshot.txt", error: true)
            return
        }

        if activeCoin != nil {
            loadCoin()
        }
        setStatus("Stock restaurado desde stock_snapshot.txt")
        registerHistory("Stock restaurado desde snapshot (macOS).")
    }

    @objc func exportReport() {
        let result = runProgvorazCLI(arguments: ["--export-report", "reporte_global.txt"])
        if result.ok {
            setStatus("Reporte generado: reporte_global.txt")
            resultView.string = result.output.isEmpty ? "Reporte global generado: reporte_global.txt" : result.output
            registerHistory("Reporte global exportado (macOS).")
        } else {
            setStatus(result.output.isEmpty ? "No se pudo generar el reporte global." : result.output, error: true)
        }
    }

    @objc func exportStockJSON() {
        let result = runProgvorazCLI(arguments: ["--export-stock-json", "stock_out.json"])
        if result.ok {
            setStatus("Stock exportado: stock_out.json")
            resultView.string = result.output.isEmpty ? "Stock exportado: stock_out.json" : result.output
            registerHistory("Stock exportado a JSON (macOS).")
        } else {
            setStatus(result.output.isEmpty ? "No se pudo exportar stock_out.json." : result.output, error: true)
        }
    }

    @objc func convertCurrency() {
        guard let destination = activeCoin, !destination.isEmpty else {
            setStatus("Primero carga la moneda destino para convertir.", error: true)
            return
        }

        let origin = originField.stringValue.trimmingCharacters(in: .whitespacesAndNewlines)
        let amount = normalize(convertAmountField.stringValue)
        if origin.isEmpty {
            setStatus("Indica la moneda origen.", error: true)
            return
        }
        if !isDigits(amount) {
            setStatus("El monto de conversion debe ser un entero no negativo en centimos.", error: true)
            return
        }

        var args = ["--convert", origin, destination, amount]
        if convertUseStockCheck.state == .on {
            args.append("--convert-stock")
        }

        let result = runProgvorazCLI(arguments: args)
        if result.ok {
            resultView.string = result.output
            convertAmountField.stringValue = ""
            setStatus("Conversion completada hacia \(destination).")
            registerHistory("Conversion macOS | \(origin)->\(destination) | Origen=\(amount) c")
        } else {
            setStatus(result.output.isEmpty ? "No se pudo completar la conversion." : result.output, error: true)
        }
    }

    func calculateLimitedChange(amount: String, minCoins: Int, maxCoins: Int) -> [String]? {
        var remain = normalize(amount)
        var solution = Array(repeating: "0", count: denoms.count)

        for i in 0..<denoms.count {
            guard let div = divmodBig(remain, denoms[i]) else { return nil }
            var take = div.q
            if compareBig(take, stock[i]) > 0 {
                take = stock[i]
            }
            solution[i] = take

            let used = mulBig(denoms[i], take)
            guard let next = subBig(remain, used) else { return nil }
            remain = next

            if remain == "0" { break }
        }

        if remain != "0" { return nil }

        var coins = 0
        for s in solution {
            if let v = Int(s) { coins += v }
        }
        if maxCoins > 0 && coins > maxCoins { return nil }
        if coins < minCoins { return nil }

        return solution
    }

    func calculateUnlimitedChange(amount: String, minCoins: Int, maxCoins: Int) -> [String]? {
        var remain = normalize(amount)
        var solution = Array(repeating: "0", count: denoms.count)

        for i in 0..<denoms.count {
            guard let div = divmodBig(remain, denoms[i]) else { return nil }
            solution[i] = div.q
            remain = div.r
            if remain == "0" { break }
        }

        if remain != "0" { return nil }

        var coins = 0
        for s in solution {
            if let v = Int(s) { coins += v }
        }
        if maxCoins > 0 && coins > maxCoins { return nil }
        if coins < minCoins { return nil }

        return solution
    }

    func parseRestriction(_ text: String) -> (Int, Int)? {
        let raw = text.replacingOccurrences(of: " ", with: "")
        if raw.isEmpty {
            return nil
        }

        if raw.first == "=" {
            let v = String(raw.dropFirst())
            guard isDigits(v), let n = Int(v), n > 0 else { return nil }
            return (n, n)
        }

        if let dash = raw.firstIndex(of: "-") {
            let left = String(raw[..<dash])
            let right = String(raw[raw.index(after: dash)...])
            guard isDigits(left), isDigits(right),
                  let minV = Int(left), let maxV = Int(right),
                  minV > 0, maxV > 0, minV <= maxV else { return nil }
            return (minV, maxV)
        }

        guard isDigits(raw), let maxV = Int(raw), maxV > 0 else { return nil }
        return (0, maxV)
    }

    func renderResult(amount: String, solution: [String]) {
        var lines: [String] = ["Cambio solicitado: \(amount) c"]
        var hasItems = false

        for i in 0..<solution.count {
            if normalize(solution[i]) == "0" { continue }
            lines.append("\(denoms[i]) c -> \(solution[i])")
            hasItems = true
        }

        if !hasItems {
            lines.append("No se requieren monedas para devolver 0.")
        }

        resultView.string = lines.joined(separator: "\n")
    }

    @objc func calculateChange() {
        guard let coin = activeCoin else {
            setStatus("Primero carga una moneda.", error: true)
            return
        }

        var amount = "0"
        if registerCheck.state == .on {
            let p = normalize(priceField.stringValue)
            let pay = normalize(paymentField.stringValue)
            if !isDigits(p) || !isDigits(pay) {
                setStatus("Precio o pago invalido.", error: true)
                return
            }
            guard let diff = subBig(pay, p) else {
                setStatus("El pago es menor que el precio.", error: true)
                return
            }
            amount = diff
        } else {
            amount = normalize(amountField.stringValue)
            if !isDigits(amount) {
                setStatus("Monto invalido: usa entero no negativo en centimos.", error: true)
                return
            }
        }

        var minCoins = 0
        var maxCoins = 0
        let l = normalize(limitField.stringValue)
        if !l.isEmpty && l != "0" {
            guard let restriction = parseRestriction(l) else {
                setStatus("Restriccion invalida. Usa N, =N o N-M.", error: true)
                return
            }
            minCoins = restriction.0
            maxCoins = restriction.1
        }

        let solution: [String]?
        if limitedMode {
            solution = calculateLimitedChange(amount: amount, minCoins: minCoins, maxCoins: maxCoins)
        } else {
            solution = calculateUnlimitedChange(amount: amount, minCoins: minCoins, maxCoins: maxCoins)
        }

        guard let solved = solution else {
            if limitedMode {
                setStatus("No existe devolucion exacta con el stock actual.", error: true)
            } else {
                setStatus("No existe devolucion exacta con denominaciones actuales.", error: true)
            }
            resultView.string = ""
            return
        }

        renderResult(amount: amount, solution: solved)

        if !limitedMode {
            amountField.stringValue = ""
            setStatus("Devolucion calculada en modo ilimitado.")
            registerHistory("Cambio aplicado (ilimitado, macOS).")
            return
        }

        var newStock = stock
        for i in 0..<newStock.count {
            guard let v = subBig(newStock[i], solved[i]) else {
                setStatus("No se pudo aplicar la devolucion al stock.", error: true)
                return
            }
            newStock[i] = v
        }

        if !updateStockSection(coin: coin, stock: newStock) {
            setStatus("No se pudo persistir la devolucion en stock.txt", error: true)
            return
        }

        stock = newStock
        table.reloadData()
        amountField.stringValue = ""
        setStatus("Devolucion aplicada y stock persistido.")
        registerHistory("Cambio aplicado con stock (macOS).")
    }

    @objc func addStock() {
        applyChange(isAdd: true)
    }

    @objc func subStock() {
        applyChange(isAdd: false)
    }
}

@main
struct ProgVorazMacOSApp {
    static func main() {
        let app = NSApplication.shared
        let delegate = AppDelegate()
        app.setActivationPolicy(.regular)
        app.delegate = delegate
        app.activate(ignoringOtherApps: true)
        app.run()
    }
}

#else
import Foundation
enum UnsupportedPlatformPlaceholder {}
#endif
