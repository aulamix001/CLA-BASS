# Correspondência com o manual fornecido

Referência: `CLA Bass.pdf`, 12 páginas, recebido nesta sessão. Os XMLs fornecem
metadados de parâmetros, componentes, presets e posições da interface; não
fornecem os algoritmos internos dos processadores listados na cadeia.

| Manual | Implementação | Validação / limite |
| --- | --- | --- |
| p. 5: estéreo e mono → estéreo | Um VST3 aceita ambos os layouts; mono é duplicado antes do DSP | Teste do wrapper e validação em host necessários |
| p. 8: Input ±10, passo 0,1, padrão 0 | Parâmetro automatizável `input` | Faixa declarada no APVTS |
| p. 8: Balance ±6 dB, passo 0,1 | `balance`; atenua o lado oposto | Atua no sinal mono duplicado também; controle só habilitado visualmente em estéreo |
| pp. 8/12: medidores −26…0, clip acima de 0, clique reseta | Pico estéreo independente; clip retido; atualização a 30 Hz | Picos capturados entre frames da GUI |
| p. 9: LED off/verde/amarelo/vermelho | Limiares próprios: −36/−18/−6 dBFS | Limiares não fornecidos no manual |
| pp. 9/10: Bass ±10; Bypass/Sub/Lower/Upper, padrão Sub | Picos EQ em 60/100/220 Hz, Q 0,7 | Frequências/Q próprios; EQ neutro a 0 testado |
| pp. 9/10: Treble ±10; Bypass/Honk/Bark/Cut, padrão Honk | Picos EQ em 900/2000/4500 Hz, Q 0,65 | Frequências/Q próprios; ganho testado |
| p. 10: Compress ±10; Bypass/Push/Spank/Wall, padrão Push | Detector estéreo ligado; razões 2/4/12; ataque 25/8/1 ms; release 180/90/50 ms | Não é emulação dos processadores internos originais; redução da dinâmica testada |
| pp. 10/11: Sub ±10; Off/On, padrão On | Realce de graves com divisor de frequência por cruzamento de zero | Design próprio; requer audição em fontes reais |
| p. 11: Distortion ±10; Mute/Growl/Roar/Rip, padrão Growl | Saturação paralela tanh, drives 2/6/14 | Design próprio, sem oversampling; aliasing em drives altos é limitação conhecida |
| pp. 11/12: Pitch ±10; Mute/Chorus/Wide/Spreader, padrão Chorus | Delay modulado estéreo com três tempos, profundidades e fases | Design próprio; não é transposição de notas |
| p. 12: Output ±10, passo 0,1, padrão 0 | Ganho final `output` | Relação −6 dB testada |
| p. 6: faders em zero, Colors ativos | Todos os faders em 0; todos os Colors em 1 | Default do manual, não o estado da imagem |
| p. 6: processamento fixo com Colors desligados | High-pass 22 Hz + curva suave de sobrecarga | Design próprio; não é bypass global |
| p. 12: presets, comparar, undo/redo, redimensionamento | Save/Load próprios, A/B, Undo/Redo, Reset | Sem compatibilidade dos presets Waves e sem redimensionamento nesta versão |

Todos os seis faders de efeito, Input e Output têm faixa −10…+10 e passo 0,1.
Defaults da fotografia podem diferir dos defaults do manual. `Reset` segue o manual.

## Sequência de aceitação Windows ainda necessária

1. Compilar com MSVC x64 usando o script fornecido ou workflow.
2. Escanear e carregar o VST3 em uma DAW Windows x64.
3. Testar pista estéreo e mono → estéreo em 44,1/48/96 kHz e blocos 32/512.
4. Executar a sequência da página 6 com uma gravação real de baixo.
5. Automatizar os quinze controles de efeito/ganho e o bypass. Salvar o projeto,
   fechar e reabrir; confirmar recuperação dos valores e do áudio.
6. Salvar/carregar preset, comparar A/B, desfazer/refazer e resetar clip.
7. Ouvir clipping, ruído nas trocas de Color, aliasing e coerência de fase em mono.
8. Integrar a imagem definitiva depois de verificar hash e dimensões, e revisar
   alinhamento de controles sem tocar nos bytes da imagem.

Sem referência de áudio e algoritmos proprietários não há comprovação de
identidade sonora. Este projeto implementa a especificação de operação publicada.
