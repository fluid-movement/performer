#!/usr/bin/env python3
"""Check that agent-docs/ has no dead links or paths and that index.md links every doc.

Run from anywhere: `python3 scripts/check-agent-docs.py` (or `just docs-check`).

What is checked
  1. Every markdown link `[text](target)` resolves (relative to the doc; #anchors ignored).
  2. Every backticked token that looks like a repo path resolves. A token looks like a path when it
     has no spaces, contains a `/` or ends in a source extension, and either starts with a known
     root (src/, tools/, agent-docs/, scripts/, ...) or one of the layer dirs (model/, engine/, ui/,
     ...), or ends in a source extension. Brace shorthand `Foo.{h,cpp}` and `Foo.h/cpp` expand to
     both files. Tokens are tried from the repo root, then under src/apps/sequencer/.
  3. Every .md file under agent-docs/ is linked from agent-docs/index.md.
"""
import os
import re
import sys

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
DOCS = os.path.join(ROOT, "agent-docs")
BASES = ["", "src/apps/sequencer/"]
EXTS = (".h", ".cpp", ".py", ".md", ".svelte", ".ts", ".txt", ".json", ".yml", ".sh")
ROOTS = ("src/", "tools/", "agent-docs/", "scripts/", "resources/", "doc/", ".claude/", "build/")
LAYER_DIRS = ("model/", "engine/", "ui/", "tests/", "python/", "core/", "platform/", "painters/", "pages/")
# Tokens that look like paths but are generated, user-local or deliberately symbolic.
IGNORE_PREFIXES = ("build/", "/tmp/", "tools/gcc-arm")


def expand(token):
    """Expand `Foo.{h,cpp}` and `Foo.h/cpp` into concrete candidate paths."""
    m = re.search(r"\{([^}]+)\}", token)
    if m:
        return [token[:m.start()] + alt + token[m.end():] for alt in m.group(1).split(",")]
    m = re.match(r"^(.*\.)h/\.?cpp$", token)
    if m:
        return [m.group(1) + "h", m.group(1) + "cpp"]
    return [token]


def exists(path):
    return any(os.path.exists(os.path.join(ROOT, base, path)) for base in BASES)


def looks_like_path(tok):
    if " " in tok or "*" in tok or "<" in tok or "::" in tok or "(" in tok or "=" in tok:
        return False
    if tok.startswith(IGNORE_PREFIXES) or tok.startswith("http"):
        return False
    if tok.startswith(ROOTS) or tok.startswith(LAYER_DIRS):
        return True
    return "/" in tok and tok.endswith(EXTS)


def main():
    errors = []
    docs = []
    for dirpath, _, files in os.walk(DOCS):
        for f in files:
            if f.endswith(".md"):
                docs.append(os.path.join(dirpath, f))

    linked_from_index = set()
    index_path = os.path.join(DOCS, "index.md")

    for doc in sorted(docs):
        rel = os.path.relpath(doc, ROOT)
        text = open(doc, encoding="utf-8").read()
        # strip fenced code blocks: they hold example code and diagrams, not references
        prose = re.sub(r"```.*?```", "", text, flags=re.S)

        for target in re.findall(r"\]\(([^)\s]+)\)", prose):
            if target.startswith(("http://", "https://", "mailto:", "#")):
                continue
            target = target.split("#")[0]
            full = os.path.normpath(os.path.join(os.path.dirname(doc), target))
            if not os.path.exists(full):
                errors.append(f"{rel}: dead link ({target})")
            elif doc == index_path:
                linked_from_index.add(full)

        for tok in re.findall(r"`([^`\n]+)`", prose):
            if not looks_like_path(tok):
                continue
            candidates = expand(tok)
            if all("/" not in c for c in candidates):
                continue  # bare file names (history prose) carry no location to check
            if not all(exists(c) for c in candidates):
                errors.append(f"{rel}: dead path `{tok}`")

    for doc in sorted(docs):
        if doc != index_path and doc not in linked_from_index:
            errors.append(f"{os.path.relpath(doc, ROOT)}: not linked from agent-docs/index.md")

    if errors:
        print("\n".join(errors))
        print(f"\n{len(errors)} problem(s) in agent-docs/")
        return 1
    print(f"agent-docs OK: {len(docs)} docs, no dead links or paths, all linked from index.md")
    return 0


if __name__ == "__main__":
    sys.exit(main())
