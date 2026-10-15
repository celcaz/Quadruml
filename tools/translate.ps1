# Preenche traduções num arquivo .ts do Qt a partir de um JSON { "texto original": "tradução" }.
# Uso: powershell -File tools\translate.ps1 i18n\quadrum_en.ts traducoes.json
param([string]$TsFile, [string]$JsonFile)

$map = Get-Content $JsonFile -Raw -Encoding UTF8 | ConvertFrom-Json
$xml = New-Object System.Xml.XmlDocument
$xml.PreserveWhitespace = $true
$xml.Load((Resolve-Path $TsFile))

$filled = 0
foreach ($message in $xml.SelectNodes('//message[not(@numerus)]')) {
    $source = $message.SelectSingleNode('source').InnerText
    $translation = $message.SelectSingleNode('translation')
    $value = $map.PSObject.Properties[$source]
    if ($value -and $translation.GetAttribute('type') -eq 'unfinished') {
        $translation.InnerText = $value.Value
        $translation.RemoveAttribute('type')
        $filled++
    }
}

$settings = New-Object System.Xml.XmlWriterSettings
$settings.Encoding = New-Object System.Text.UTF8Encoding $false
$path = (Resolve-Path $TsFile).Path
$writer = [System.Xml.XmlWriter]::Create($path, $settings)
$xml.Save($writer)
$writer.Close()

# O XmlWriter escreve "<!DOCTYPE TS []>"; o formato do Qt usa "<!DOCTYPE TS>".
$text = [IO.File]::ReadAllText($path).Replace('<!DOCTYPE TS []>', '<!DOCTYPE TS>')
[IO.File]::WriteAllText($path, $text, $settings.Encoding)
"translations filled: $filled"
