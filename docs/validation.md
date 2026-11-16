# Validação desta implementação

## Executado com sucesso no ambiente Linux

- `bash scripts/setup-cloud.sh`: instalação local repetível, JUCE 8.0.6,
  compilação completa de VST3 e Standalone e CTest: **2/2 suítes passaram**.
- `BassDSP`: silêncio, bypass exato, EQ neutro em zero, boost/cut, ganho de saída,
  redução de dinâmica, atuação dos efeitos, simetria estéreo, invariância entre
  blocos e áudio finito em 44,1/48/96/192 kHz.
- `BassProcessor`: 16 parâmetros, faixas/defaults, layouts, recuperação do estado,
  rejeição de estado inválido, bypass, clip e roteamento mono → estéreo.
- DSP compilado e executado com AddressSanitizer e UndefinedBehaviorSanitizer:
  passou sem erros reportados.
- Editor criado e renderizado por JUCE a 900 × 570: passou. Isso verifica a
  interface provisória, não a integração com a imagem definitiva.
- O helper VST3 carregou o módulo Linux e gerou `moduleinfo.json` com os
  componentes de áudio e controle.

## Windows

### Investigação do fechamento ao inserir no REAPER

O build original passou no pluginval 1.0.4, nível 5, em Windows x64 no run
[`43933617172`](https://github.com/aulamix001/CLA-BASS/actions/runs/43933617172),
incluindo editor, abertura durante processamento e automação. Também passou no
Linux com janela nativa sob Xvfb. Isso não reproduz nem explica a falha específica
do usuário. Não há crash dump ou código de exceção para atribuir uma causa exata.

A revisão 0.1.1 seleciona a renderização GDI por nome na API pública do JUCE,
após o host criar o peer da janela, e incorpora o CRT do MSVC estaticamente.
Não altera `BassDsp.h`, IDs VST3, parâmetros, presets ou arquivos de imagem.

Novas barreiras no workflow antes da publicação:

- `BassEditorLifecycle`: 12 ciclos de abrir/automatizar/restaurar/fechar a janela
  nativa durante áudio em uma thread separada; confirma o backend GDI no Windows.
- pluginval 1.0.4, nível 5, com GUI habilitada, seed 12345.
- `test-reaper-windows.ps1`: REAPER 7.86 x64 oficial com assinatura Authenticode
  verificada; configuração descartável e três ciclos de inserir VST3, abrir a
  interface, automatizar, reabrir e remover. Não usa configurações do usuário.

O resultado da máquina Windows hospedada não comprova compatibilidade com o
driver gráfico ou de áudio específico do usuário. Studio One permanece sem teste.

O script PowerShell e o workflow usam MSVC 2022 x64. O workflow verifica a
existência do binário em `Contents/x86_64-win`, executa as suítes e empacota a
pasta VST3 inteira antes de publicar o ZIP em uma release associada ao commit.

O resultado da execução Windows deve ser confirmado no GitHub Actions; não pode
ser inferido de uma compilação Linux. JUCE 8 rejeita MinGW explicitamente.
Carregamento, audição e interação em uma DAW Windows ainda exigem o roteiro
de aceitação em `manual-mapping.md`, mesmo se a compilação e os testes passarem.

## Pendente

A imagem definitiva ainda não foi fornecida como arquivo. Nenhum anexo foi
alterado. A interface final e a equivalência sonora com o plugin original não
foram validadas; os algoritmos de DSP deste projeto são próprios.
