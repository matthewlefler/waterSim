using Microsoft.Xna.Framework;
using Microsoft.Xna.Framework.Graphics;
using Microsoft.Xna.Framework.Input;

using Messengers;
using System.Reflection;
using System.Data.Common;

namespace frontend;

public class Game1 : Game
{
    private GraphicsDeviceManager _graphics;
    private SpriteBatch _spriteBatch;

    Texture2D tex;

    private Messenger messenger;

    private Vector4[,] sim = new Vector4[0,0];

    public Game1()
    {
        _graphics = new GraphicsDeviceManager(this);
        Content.RootDirectory = "Content";
        IsMouseVisible = true;
    }

    protected override void Initialize()
    {
        // TODO: Add your initialization logic here
        messenger = new Messenger(4331);
        messenger.connect();

        base.Initialize();
    }

    protected override void LoadContent()
    {
        _spriteBatch = new SpriteBatch(GraphicsDevice);

        // TODO: use this.Content to load your game content here
        tex = Content.Load<Texture2D>("white");
    }


    float time = 0;
    float dt = 0;
    protected override void Update(GameTime gameTime)
    {
        if (GamePad.GetState(PlayerIndex.One).Buttons.Back == ButtonState.Pressed || Keyboard.GetState().IsKeyDown(Keys.Escape))
            Exit();

        dt = (float) gameTime.ElapsedGameTime.TotalSeconds;

        if(messenger.connected) { sim = messenger.read(); }
        else { 
            if(time < 10)
            {
                time += dt;
            }
            else
            {
                time = 0;
                messenger.connect(); 
            }
        }

        // TODO: Add your update logic here

        base.Update(gameTime);
    }

    const int squareWidth = 20;
    const int squareGap = squareWidth/4;
    protected override void Draw(GameTime gameTime)
    {
        GraphicsDevice.Clear(Color.CornflowerBlue);

        _spriteBatch.Begin();

        // TODO: Add your drawing code here
        for (int x = 0; x < sim.GetLength(0); x++)
        {
            for (int y = 0; y < sim.GetLength(1); y++)
            {
                Vector4 point = sim[x,y];
                _spriteBatch.Draw(tex, new Rectangle((int) point.X * (squareWidth + squareGap), (int) point.Y * (squareWidth + squareGap), squareWidth, squareWidth), new Color(point.W, point.W, point.W));        
            }
        }

        _spriteBatch.End();
        base.Draw(gameTime);
    }

    protected override void EndRun()
    {
        if(messenger.connected)
        {
            messenger.close();
        }

        base.EndRun();
    }
}
