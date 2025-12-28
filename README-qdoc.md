# Generate doc

## Dependencies

We need to clone kde-qdoc-common

```bash
cd <kde path>
git clone https://invent.kde.org/sdk/kde-qdoc-common
```

## Environment variables

set in `.bashrc`

```bash
export KDE_DOCS=<kde path>/kde-qdoc-common
```

We need to define where we generate doc:

```bash
export GENERATE_DOCDIR=<kde path>/generate-docs
```

## CMakePreset.json

For generating doc we have a target in CMakePreset.json

```bash
cmake --preset generate-doc

cmake --build --preset generate-doc
```

we can show result in `$GENERATE_DOCDIR/`

## Porting info

[Streamlined Application Development Experience #10](https://invent.kde.org/teams/goals/streamlined-application-development-experience/-/issues/10)
