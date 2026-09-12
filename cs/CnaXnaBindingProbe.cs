using Microsoft.Xna.Framework;

namespace CnaStarfield;

// Compile-time proof that this project can consume the real XNA-shaped CNA facade.
static class CnaXnaBindingProbe
{
    public static Vector3 CourierOrigin => Vector3.Zero;
}