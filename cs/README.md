# C# port

This is a headless port of the shared contract. Run `dotnet run --project
cs -- startup` (or another scenario such as `win`) from the repository root.
The future graphical shell is deliberately kept behind the XNA 4.0-shaped
contract; it must not use `CNA.Ext`.

The headless runner does not claim to be backed by CNA.NET. To compile the
separate XNA-shaped binding probe against the local `cna-cs` repository, run
from the repository root:

```sh
dotnet build cs/Starfield.csproj -c Release \
  -p:UseCnaCs=true -p:CnaCsRoot="$PWD/../cna-cs"
```

This references `CNA.XnaCompat`, which exposes the idiomatic
`Microsoft.Xna.Framework` surface. It is a compile probe, not yet the game's
runtime or rendering integration.