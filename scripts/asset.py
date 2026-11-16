"""Byte-preserving import and verification of the definitive PNG. Never converts an image."""
import argparse
import hashlib
import json
from pathlib import Path
import struct

ROOT = Path(__file__).resolve().parents[1]
DEST = ROOT / "assets" / "definitive.png"
MANIFEST = ROOT / "assets" / "definitive.json"

def fingerprint(data):
    if data[:8] != b"\x89PNG\r\n\x1a\n" or data[12:16] != b"IHDR":
        raise SystemExit("Forneça o PNG original. Nenhuma conversão será feita.")
    w, h = struct.unpack(">II", data[16:24])
    return dict(sha256=hashlib.sha256(data).hexdigest(), width=w, height=h, bytes=len(data))

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("command", choices=["import", "verify"])
    parser.add_argument("source", nargs="?")
    args = parser.parse_args()
    if args.command == "import":
        if not args.source:
            parser.error("Informe o caminho do PNG definitivo.")
        if DEST.exists() or MANIFEST.exists():
            raise SystemExit("Arquivo definitivo já registrado. Sobrescrita proibida.")
        data = Path(args.source).read_bytes()
        info = fingerprint(data)
        with DEST.open("xb") as out:
            out.write(data)
        with MANIFEST.open("x", encoding="utf-8") as out:
            json.dump(info, out, indent=2)
        DEST.chmod(0o444)
    if not DEST.exists() or not MANIFEST.exists():
        raise SystemExit("PNG definitivo e registro de integridade ainda não fornecidos.")
    expected = json.loads(MANIFEST.read_text(encoding="utf-8"))
    actual = fingerprint(DEST.read_bytes())
    if expected != actual:
        raise SystemExit("ERRO: conteúdo ou dimensões da imagem definitiva mudaram.")
    print("Imagem definitiva verificada:", actual)

if __name__ == "__main__":
    main()
