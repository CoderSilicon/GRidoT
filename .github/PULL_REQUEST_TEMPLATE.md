name: Pull request

description: What this pull request changes

body:
  - type: input
    id: title
    attributes:
      label: Summary
      description: One line, imperative, no trailing period
      placeholder: Add --limit cap to the orphan detail list
    validations:
      required: true

  - type: textarea
    id: description
    attributes:
      label: Description
      description: What changed and why. Link any issue, e.g. Fixes #42.
    validations:
      required: true

  - type: dropdown
    id: commands
    attributes:
      label: Checklist
      options:
        - label: make format
        - label: make lint
        - label: make debug
        - label: make asan
        - label: Tests added or updated
        - label: Documentation updated (README, docs/, man page)
        - label: No system state is mutated (still read-only)

  - type: checkboxes
    id: scope
    attributes:
      label: Scope
      options:
        - label: This pull request does not install or remove packages.
          required: true
        - label: This pull request adds no new runtime dependency.
          required: true