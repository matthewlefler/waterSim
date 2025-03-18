using Cameras;
using Microsoft.Xna.Framework.Content;
using Microsoft.Xna.Framework.Graphics;
using Microsoft.Xna.Framework.Input;

namespace Levels;

public interface ILevel
{
    /// <summary>
    /// Runs any intilizing scripts, function, and such to set up the level
    /// 
    /// Called whenever a level is switched / loaded
    /// </summary>
    public void init();

    /// <summary>
    /// Load any content required for the level to run
    /// </summary>
    /// <param name="content"> the base monogame content manager </param>
    /// <param name="graphics_device"> at least one monogame extended function requires the graphics device </param>
    public void load_content(ContentManager content, GraphicsDevice graphics_device);

    /// <summary>
    /// The update loop
    /// </summary>
    /// <param name="dt"> delta time in seconds </param>
    /// <param name="keyboard_state"> the current frame's state of the user's keyboard </param>
    /// <param name="last_keyboard_state"> the last frame's state of the user's keyboard </param>
    public void update(float dt, KeyboardState keyboard_state, KeyboardState last_keyboard_state, Camera camera);

    /// <summary>
    /// The draw function,
    /// Put any code related to generating the next frame in it
    /// </summary>
    /// <param name="graphics_device"></param>
    /// <param name="sprite_batch"></param>
    public void draw(BasicEffect effect, GraphicsDevice graphics_device, SpriteBatch sprite_batch, Camera camera);

    /// <summary>
    /// run any closing scripts before the program exits
    /// </summary>
    public void close();

    /// <summary>
    /// Return the name of this level
    /// </summary>
    /// <returns> the name of this level object </returns>
    public string getName();
} 