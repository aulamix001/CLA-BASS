O PNG definitivo ainda não está disponível como arquivo neste checkout.
Não substituir por outra geração nem extrair uma imagem diferente do PDF.

Quando o arquivo original for fornecido, registrar uma única vez:

```sh
python scripts/asset.py import /caminho/da/imagem-original.png
python scripts/asset.py verify
```

O importador copia bytes, sem redimensionar ou recomprimir. Recusa sobrescrita,
registra SHA-256 e dimensões e marca o PNG como somente leitura. O CMake valida
o registro antes de embutir o arquivo. Isso detecta alterações acidentais; não
substitui permissões de acesso no sistema operacional.

A interface provisória é desenhada em C++ em arquivo separado. Integrar e
validar as posições dos controles quando a imagem definitiva for recebida;
o registro do PNG, por si só, não conclui essa integração visual.
