These test-only recognizers preserve ReSkate baseline commit
`35693567da38795c8d73b32fddb30c3b30267d2b` for identical-trace differential
checks. Only namespaces and dependency paths are adjusted. They are excluded
from production includes and are never a runtime fallback.

Older component tests exercise these frozen contracts. Production acceptance
is exercised separately by the input events, input session, manual input,
manual differential and shared controller replay tests.
