#!/usr/bin/env bash
# Roda todos os testes automáticos antes de publicar. Uso: tests/rodar_tudo.sh   (na raiz do repositório)
cd "$(dirname "$0")/.." || exit 1
ok=0; falha=0; T=$(mktemp -d)
python3 tools/versionar_sw.py   # carimba o cache do service worker com o conteúdo atual
rodar(){ local nome="$1"; shift; if timeout 240 "$@" >"$T/o.txt" 2>&1 && ! grep -qiE "traceback|FALHA|SILENCIO|CLIP|erros \[[^]]" "$T/o.txt"; then echo "  ok   $nome"; ok=$((ok+1)); else echo "  FALHOU $nome"; tail -5 "$T/o.txt" | sed 's/^/       /'; falha=$((falha+1)); fi; }
(python3 -m http.server 8765 >/dev/null 2>&1 &) ; SP=$!; sleep 1
echo "Motor (Node):";  for f in tests/worklet_unit.js tests/worklet_filtros_lfo2.js; do rodar "$f" node "$f"; done
echo "Motor C++:";     for f in tests/motor_cpp_efeitos tests/motor_cpp_filtros tests/motor_cpp_modmix tests/motor_cpp_unissono_anel tests/motor_cpp_presets_arp; do g++ -O2 -std=c++17 -Ivst/Source "$f.cpp" -o "$T/t" 2>"$T/e.txt" && rodar "$f" "$T/t" || { echo "  FALHOU $f (compilação)"; falha=$((falha+1)); }; done
echo "Plugin x app (Node):"; rodar tests/plugin_presets.js node tests/plugin_presets.js
echo "Interface (Playwright):"; for f in tests/ao_vivo.py tests/efeitos_jdxi.py tests/ux_apagar_presets.py tests/ux_setlist_categorias.py tests/preset_efeitos.py tests/novidades.py tests/orientacao.py tests/midi_duplicado.py tests/exportar_teste.py tests/trial_email.py; do rodar "$f" python3 "$f"; done
pkill -f "http.server 8765" 2>/dev/null
echo; echo "Resultado: $ok ok, $falha com falha"; rm -rf "$T"; [ "$falha" -eq 0 ]
