# Operating Systems Course (SUT) — xv6-riscv

Homeworks (2-person team) for the Operating Systems course, built on a pinned
commit of MIT's [xv6-riscv](https://github.com/mit-pdos/xv6-riscv).
The original xv6 readme is kept in `README.xv6`.

## Branch / tag model

| Ref | Meaning |
|-----|---------|
| `xv6-base` (tag) | The pristine xv6 commit the course pins (`c71a6c4`) |
| `main` | Docs (`docs/hwN/`), this README, shared config. Kernel here stays at `xv6-base` unless a homework builds on the previous one |
| `hwN` | Kernel work for homework N, branched from `xv6-base` (or from `hw(N-1)` if the homework is cumulative) |
| `hwN/<task>` | Short-lived task branches, merged into `hwN` through a PR |
| `hwN-submission` (tag) | Exactly what was handed in |

Never rebase published branches. If a homework needs the original kernel, branch
from `xv6-base`; earlier homeworks stay reachable via their branch/tag.

## Layout

```
docs/hwN/latex/       LaTeX answers (build: cd docs/hwN/latex && latexmk -xelatex main.tex)
docs/hwN/pre_provided handout and course material
kernel/ user/ ...     xv6 sources (on the hwN branches)
```

## Commit style

`hw1: add trace syscall`, `docs(hw1): problem 2 answer`.
