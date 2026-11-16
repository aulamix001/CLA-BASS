# Foundation Bass — C++ / JUCE, Windows x64

Plugin de baixo em VST3 e aplicativo Standalone. Implementação própria baseada
nos controles e no comportamento descritos no manual CLA Bass fornecido pelo
usuário. Não contém o DSP da Waves nem exige WaveShell. Não há equivalência
sonora comprovada com o produto original.

### Revisão de compatibilidade 0.1.1

A janela Windows usa o backend **Software Renderer / GDI** do JUCE, selecionado
quando o host associa a janela nativa ao editor, em vez de depender da GPU via
Direct2D. O runtime C++ é incorporado estaticamente no build MSVC. O DSP, os IDs
de automação, o estado dos presets e a imagem definitiva não foram alterados.

O fechamento relatado no REAPER não foi reproduzido no validador da versão
original; essas alterações tratam dependências de compatibilidade, sem atribuir
uma causa confirmada ao computador do usuário. O workflow inclui pluginval nível
5, ciclos de janela nativa durante áudio e um teste com REAPER 7.86 x64 isolado.
Studio One ainda não foi testado.

Ao atualizar, feche a DAW e substitua a **pasta `.vst3` completa**. Evite manter
uma segunda cópia antiga em outro diretório de plugins. Atualize a lista de
plugins na DAW antes de testar novamente.

## Compilar no Windows 64 bits

1. Instale Visual Studio 2022 com **Desktop development with C++**, MSVC e Windows
   SDK, CMake 3.22+ e Git.
2. Na pasta deste projeto, execute no PowerShell:

   ```powershell
   powershell -ExecutionPolicy Bypass -File scripts/build-windows.ps1
   ```

   O CMake busca o JUCE **8.0.6**, fixado no commit
   `51a8a6d7aeae7326956d747737ccf1575e61e209`. Para um checkout local,
   passe `-DJUCE_DIR=C:/caminho/JUCE` ao configurar.
3. O bundle estará em
   `build-windows/FoundationBass_artefacts/Release/VST3/Foundation Bass.vst3`.
   Copie a **pasta .vst3 completa** para
   `C:\Program Files\Common Files\VST3\` e atualize a lista de plugins na DAW.
4. O aplicativo para teste sem DAW estará na subpasta `Standalone`.

O workflow `.github/workflows/windows.yml` executa a compilação MSVC x64 e os
testes no GitHub Actions. Após sucesso em `main`, publica o ZIP do VST3 em uma
release com tag `windows-<commit>`. Resultados locais estão em
[docs/validation.md](docs/validation.md); confirme cada build Windows no Actions.

## Usar, na ordem do manual

1. Insira na pista de baixo. São aceitos estéreo → estéreo e mono → estéreo.
2. Ajuste **Input Sens** observando o LED e o pico de entrada. Os limiares de LED
   desta implementação são −36, −18 e −6 dBFS; o manual não especifica esses valores.
3. Ajuste **Bass** e **Treble** e selecione os Colors. Em zero, trocar o Color do
   EQ não muda a resposta do filtro.
4. Ajuste **Compress** para controlar a dinâmica. Depois ajuste **Sub**,
   **Distortion** e **Pitch**. Esses quatro módulos já atuam com o fader em zero.
5. Reduza **Output** se houver clip. Clique em um medidor para apagar os indicadores.
6. Use **Save/Load** para presets próprios `.fbpreset`, **A/B** para comparar,
   **Undo/Redo** para desfazer/refazer e **Reset** para o padrão documentado.
   A DAW salva parâmetros automaticamente no projeto. Bypass global é transparente.

Todas as faixas e diferenças em relação ao manual estão em
[docs/manual-mapping.md](docs/manual-mapping.md).

## Testes sem dependências de interface

```sh
cmake -S . -B build-dsp -DBASS_DSP_ONLY=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build-dsp --parallel 2
ctest --test-dir build-dsp --output-on-failure
```

Os testes exercitam áudio real: silêncio, bypass, ganho, EQ, compressão,
efeitos paralelos, estabilidade em quatro taxas e invariância ao tamanho do bloco.
Eles não substituem a validação de carregamento e audição em uma DAW Windows.

## Imagem definitiva

Nenhuma imagem foi recriada, convertida ou sobrescrita durante a implementação.
O arquivo definitivo ainda precisa ser fornecido. A interface atual é provisória.
Veja [assets/README.md](assets/README.md) para importar bytes sem alteração e
registrar SHA-256 e dimensões. A integração visual final precisa ser validada
depois de receber esse arquivo.

## Desenvolvimento

Na máquina de nuvem Debian 13 desta sessão, `bash scripts/setup-cloud.sh` prepara
ferramentas locais em `/workspace`, compila a versão Linux de validação e executa
os testes. Ele não instala nem produz uma versão Windows. O desenvolvimento
Windows usa MSVC conforme as instruções acima; JUCE 8 rejeita MinGW.

O processamento de áudio não depende da interface (`Source/BassDsp.h`). O wrapper
JUCE fornece automação e persistência. Memória de delay é alocada em
`prepareToPlay`, fora do callback. Faders usam suavização de 20 ms; medidores são
transmitidos por atomics. Cada tarefa na nuvem já é isolada: use o checkout
existente; não crie worktrees sem solicitação explícita.

O JUCE é uma dependência com termos próprios (AGPLv3 ou licença comercial).
Escolha a licença aplicável antes de distribuir. Os XMLs e o PDF fornecidos
foram usados como referência, sem redistribuí-los no pacote do plugin.
