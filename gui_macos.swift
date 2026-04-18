#if os(macOS)
import Cocoa

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

final class AppDelegate: NSObject, NSApplicationDelegate, NSTableViewDataSource {
    var window: NSWindow!
    var coinPopup: NSPopUpButton!
    var denomPopup: NSPopUpButton!
    var qtyField: NSTextField!
    var amountField: NSTextField!
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
        window.title = "ProgVoraz GUI macOS (Swift)"
        window.center()

        let content = window.contentView!

        let title = NSTextField(labelWithString: "Panel GUI (macOS) - Modos")
        title.frame = NSRect(x: 20, y: 600, width: 360, height: 24)
        title.font = NSFont.boldSystemFont(ofSize: 16)
        content.addSubview(title)

        let coinLabel = NSTextField(labelWithString: "Moneda")
        coinLabel.frame = NSRect(x: 20, y: 568, width: 60, height: 22)
        content.addSubview(coinLabel)

        coinPopup = NSPopUpButton(frame: NSRect(x: 85, y: 565, width: 180, height: 26), pullsDown: false)
        content.addSubview(coinPopup)

        let loadBtn = NSButton(title: "Cargar", target: self, action: #selector(loadCoin))
        loadBtn.frame = NSRect(x: 275, y: 564, width: 90, height: 28)
        content.addSubview(loadBtn)

        let reloadBtn = NSButton(title: "Recargar", target: self, action: #selector(reloadCoinsAction))
        reloadBtn.frame = NSRect(x: 375, y: 564, width: 90, height: 28)
        content.addSubview(reloadBtn)

        modeLimitedBtn = NSButton(radioButtonWithTitle: "Stock limitado", target: self, action: #selector(modeChanged(_:)))
        modeLimitedBtn.frame = NSRect(x: 20, y: 532, width: 140, height: 22)
        modeLimitedBtn.state = .on
        content.addSubview(modeLimitedBtn)

        modeUnlimitedBtn = NSButton(radioButtonWithTitle: "Stock ilimitado", target: self, action: #selector(modeChanged(_:)))
        modeUnlimitedBtn.frame = NSRect(x: 170, y: 532, width: 150, height: 22)
        content.addSubview(modeUnlimitedBtn)

        let scroll = NSScrollView(frame: NSRect(x: 20, y: 290, width: 740, height: 230))
        table = NSTableView(frame: scroll.bounds)

        let col1 = NSTableColumn(identifier: NSUserInterfaceItemIdentifier("denom"))
        col1.title = "Denominacion"
        col1.width = 300

        let col2 = NSTableColumn(identifier: NSUserInterfaceItemIdentifier("stock"))
        col2.title = "Stock"
        col2.width = 420

        table.addTableColumn(col1)
        table.addTableColumn(col2)
        table.dataSource = self

        scroll.documentView = table
        scroll.hasVerticalScroller = true
        content.addSubview(scroll)

        let denomLabel = NSTextField(labelWithString: "Denominacion")
        denomLabel.frame = NSRect(x: 20, y: 250, width: 90, height: 22)
        content.addSubview(denomLabel)

        denomPopup = NSPopUpButton(frame: NSRect(x: 115, y: 247, width: 180, height: 26), pullsDown: false)
        content.addSubview(denomPopup)

        let qtyLabel = NSTextField(labelWithString: "Cantidad")
        qtyLabel.frame = NSRect(x: 310, y: 250, width: 70, height: 22)
        content.addSubview(qtyLabel)

        qtyField = NSTextField(frame: NSRect(x: 385, y: 246, width: 160, height: 26))
        content.addSubview(qtyField)

        addBtn = NSButton(title: "Anadir", target: self, action: #selector(addStock))
        addBtn.frame = NSRect(x: 230, y: 204, width: 100, height: 30)
        content.addSubview(addBtn)

        subBtn = NSButton(title: "Quitar", target: self, action: #selector(subStock))
        subBtn.frame = NSRect(x: 340, y: 204, width: 100, height: 30)
        content.addSubview(subBtn)

        let specificTitle = NSTextField(labelWithString: "Cambio especifico (cantidades separadas por espacios, en orden de denominaciones)")
        specificTitle.frame = NSRect(x: 20, y: 168, width: 720, height: 22)
        content.addSubview(specificTitle)

        let specificInLabel = NSTextField(labelWithString: "Entregado")
        specificInLabel.frame = NSRect(x: 20, y: 142, width: 80, height: 22)
        content.addSubview(specificInLabel)

        specificInField = NSTextField(frame: NSRect(x: 105, y: 138, width: 560, height: 26))
        content.addSubview(specificInField)

        let specificOutLabel = NSTextField(labelWithString: "Devolucion")
        specificOutLabel.frame = NSRect(x: 20, y: 112, width: 80, height: 22)
        content.addSubview(specificOutLabel)

        specificOutField = NSTextField(frame: NSRect(x: 105, y: 108, width: 560, height: 26))
        content.addSubview(specificOutField)

        specificBtn = NSButton(title: "Aplicar cambio especifico", target: self, action: #selector(applySpecificChange))
        specificBtn.frame = NSRect(x: 670, y: 108, width: 90, height: 56)
        content.addSubview(specificBtn)

        let amountLabel = NSTextField(labelWithString: "Monto (centimos)")
        amountLabel.frame = NSRect(x: 20, y: 70, width: 110, height: 22)
        content.addSubview(amountLabel)

        amountField = NSTextField(frame: NSRect(x: 140, y: 66, width: 220, height: 26))
        content.addSubview(amountField)

        let calcBtn = NSButton(title: "Calcular devolucion", target: self, action: #selector(calculateChange))
        calcBtn.frame = NSRect(x: 370, y: 64, width: 170, height: 30)
        content.addSubview(calcBtn)

        let resultTitle = NSTextField(labelWithString: "Resultado")
        resultTitle.frame = NSRect(x: 20, y: 42, width: 120, height: 22)
        content.addSubview(resultTitle)

        let resultScroll = NSScrollView(frame: NSRect(x: 20, y: 44, width: 740, height: 90))
        resultView = NSTextView(frame: resultScroll.bounds)
        resultView.isEditable = false
        resultView.font = NSFont.monospacedSystemFont(ofSize: 12, weight: .regular)
        resultScroll.documentView = resultView
        resultScroll.hasVerticalScroller = true
        content.addSubview(resultScroll)

        statusLabel = NSTextField(labelWithString: "")
        statusLabel.frame = NSRect(x: 20, y: 12, width: 740, height: 24)
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
        return limitedMode ? stock[row] : "Ilimitado"
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
            setStatus("No se pudieron leer monedas desde monedas.txt", error: true)
        } else {
            setStatus("Monedas cargadas. Selecciona una y pulsa Cargar.")
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
        setStatus(limitedMode ? "Modo stock limitado activo." : "Modo stock ilimitado activo.")
    }

    @objc func loadCoin() {
        guard let coin = coinPopup.titleOfSelectedItem, !coin.isEmpty else {
            setStatus("Selecciona una moneda valida.", error: true)
            return
        }

        guard let d = loadSection(file: "monedas.txt", coin: coin, minusOneAsZero: false),
              let s = loadSection(file: "stock.txt", coin: coin, minusOneAsZero: true),
              d.count == s.count else {
            setStatus("No se pudo cargar denominaciones/stock.", error: true)
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
        setStatus("Moneda \(coin) cargada.")
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
        lines.append(unlimited ? "Cambio especifico aplicado (ilimitado): \(total) c" : "Cambio especifico aplicado (limitado): \(total) c")
        var hasItems = false
        for i in 0..<requested.count {
            if normalize(requested[i]) == "0" { continue }
            lines.append("\(denoms[i]) c -> \(requested[i])")
            hasItems = true
        }
        if !hasItems {
            lines.append("No se requieren monedas para devolver 0.")
        }
        resultView.string = lines.joined(separator: "\n")
    }

    @objc func applySpecificChange() {
        guard let coin = activeCoin else {
            setStatus("Primero carga una moneda.", error: true)
            return
        }

        guard let given = parseSpecificList(specificInField.stringValue, expected: denoms.count),
              let requested = parseSpecificList(specificOutField.stringValue, expected: denoms.count) else {
            setStatus("Formato invalido: usa exactamente una cantidad por denominacion en ambos campos.", error: true)
            return
        }

        let totalGiven = totalValue(given)
        let totalRequested = totalValue(requested)
        if compareBig(totalGiven, totalRequested) != 0 {
            setStatus("Total entregado (\(totalGiven) c) y total solicitado (\(totalRequested) c) deben ser iguales.", error: true)
            return
        }

        if !limitedMode {
            renderSpecificResult(total: totalGiven, requested: requested, unlimited: true)
            setStatus("Cambio especifico aplicado en modo ilimitado.")
            return
        }

        var newStock = stock
        for i in 0..<newStock.count {
            let afterIn = addBig(newStock[i], given[i])
            guard let afterOut = subBig(afterIn, requested[i]) else {
                setStatus("No se puede aplicar la devolucion especifica con el stock disponible.", error: true)
                return
            }
            newStock[i] = afterOut
        }

        if !updateStockSection(coin: coin, stock: newStock) {
            setStatus("No se pudo persistir el cambio especifico en stock.txt", error: true)
            return
        }

        stock = newStock
        table.reloadData()
        renderSpecificResult(total: totalGiven, requested: requested, unlimited: false)
        setStatus("Cambio especifico aplicado y stock persistido.")
    }

    func applyChange(isAdd: Bool) {
        if !limitedMode {
            setStatus("Panel administrador disponible solo en modo stock limitado.", error: true)
            return
        }

        guard let coin = activeCoin else {
            setStatus("Primero carga una moneda.", error: true)
            return
        }

        let idx = denomPopup.indexOfSelectedItem
        if idx < 0 || idx >= denoms.count {
            setStatus("Selecciona una denominacion valida.", error: true)
            return
        }

        let qty = normalize(qtyField.stringValue)
        if !isDigits(qty) {
            setStatus("Cantidad invalida: usa entero no negativo.", error: true)
            return
        }

        let old = stock[idx]
        if isAdd {
            stock[idx] = addBig(stock[idx], qty)
        } else {
            guard let v = subBig(stock[idx], qty) else {
                setStatus("No puedes quitar mas stock del disponible.", error: true)
                return
            }
            stock[idx] = v
        }

        if !updateStockSection(coin: coin, stock: stock) {
            stock[idx] = old
            setStatus("No se pudo persistir stock en stock.txt", error: true)
            return
        }

        qtyField.stringValue = ""
        table.reloadData()
        setStatus(isAdd ? "Stock actualizado (suma)." : "Stock actualizado (resta).")
    }

    func calculateLimitedChange(amount: String) -> [String]? {
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

        return remain == "0" ? solution : nil
    }

    func calculateUnlimitedChange(amount: String) -> [String]? {
        var remain = normalize(amount)
        var solution = Array(repeating: "0", count: denoms.count)

        for i in 0..<denoms.count {
            guard let div = divmodBig(remain, denoms[i]) else { return nil }
            solution[i] = div.q
            remain = div.r
            if remain == "0" { break }
        }

        return remain == "0" ? solution : nil
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

        let amount = normalize(amountField.stringValue)
        if !isDigits(amount) {
            setStatus("Monto invalido: usa entero no negativo en centimos.", error: true)
            return
        }

        let solution: [String]?
        if limitedMode {
            solution = calculateLimitedChange(amount: amount)
        } else {
            solution = calculateUnlimitedChange(amount: amount)
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
