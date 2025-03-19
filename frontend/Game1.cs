using System;

using Microsoft.Xna.Framework;
using Microsoft.Xna.Framework.Graphics;
using Microsoft.Xna.Framework.Input;

using MonoGame.Extended.BitmapFonts;

using Messengers;
using Cameras;
using Objects;
using Levels;

namespace frontend;

public class Game1 : Game
{
    private GraphicsDeviceManager _graphics;
    private SpriteBatch _spriteBatch;

    private int screen_x;
    private int screen_y;
    Point screen_middle;

    VertexPositionColor[] origin_axii_lines = new VertexPositionColor[]
    {
        new VertexPositionColor(Vector3.Up, Color.Red),
        new VertexPositionColor(Vector3.Zero, Color.Red),
        
        new VertexPositionColor(Vector3.Right, Color.Green),
        new VertexPositionColor(Vector3.Zero, Color.Green),
        
        new VertexPositionColor(Vector3.Forward, Color.Blue),
        new VertexPositionColor(Vector3.Zero, Color.Blue),
    };

    Texture2D tex;
    BitmapFont font;

    BasicEffect basicEffect;


    SimpleCamera camera;

    private ILevel current_level;

    public Game1()
    {
        _graphics = new GraphicsDeviceManager(this);
        Content.RootDirectory = "Content";
        IsMouseVisible = false;
    }

    public void setLevel(ILevel level)
    {
        this.current_level = level;
    }

    protected override void Initialize()
    {
        _graphics.PreferredBackBufferHeight = 1300;
        _graphics.PreferredBackBufferWidth = 1300;
        _graphics.ApplyChanges();
        
        
        screen_x = _graphics.PreferredBackBufferWidth;
        screen_y = _graphics.PreferredBackBufferHeight;
    
        screen_middle = new Point(screen_x / 2, screen_y / 2);

        Console.WriteLine("screen width: " + screen_x + " screen height: " + screen_y);

        basicEffect = new BasicEffect(GraphicsDevice); //basic effect
        
        basicEffect.VertexColorEnabled = true;
        basicEffect.AmbientLightColor = new Vector3(0.5f,0.5f,0.5f);
        basicEffect.EmissiveColor = new Vector3(0.5f, 0.5f, 0.5f);


        // TODO: Add your initialization logic here
        current_level = new LevelSelector(screen_x, screen_y, this);

        camera = new SimpleCamera(Vector3.Backward * 2);

        LevelSelector levelSelector = new LevelSelector(screen_x, screen_y, this);

        levelSelector.addLevel(new ReadFileLevel("/home/lefler/Documents/gitRepos/waterSim/backend/build/test.txt", GraphicsDevice));

        levelSelector.addLevel(new NetworkedSimulation(GraphicsDevice));

        current_level = levelSelector;        

        current_level.init();


        base.Initialize();

        Console.WriteLine("Initialization done");
    }

    protected override void LoadContent()
    {
        _spriteBatch = new SpriteBatch(GraphicsDevice);

        current_level.load_content(Content, GraphicsDevice);

        // TODO: use this.Content to load your game content here
        tex = Content.Load<Texture2D>("white");
        font = MonoGame.Extended.BitmapFonts.BitmapFont.FromFile(GraphicsDevice, "Content/sans-serif.fnt");
    }


    float dt = 0; // delta time in seconds

    MouseState last_mouse = Mouse.GetState();
    MouseState mouse = Mouse.GetState();
    KeyboardState last_keyboard = Keyboard.GetState();
    KeyboardState keyboard = Keyboard.GetState();

    protected override void Update(GameTime gameTime)
    {
        dt = (float) gameTime.ElapsedGameTime.TotalSeconds;

        last_keyboard = keyboard;
        keyboard = Keyboard.GetState();
        last_mouse = mouse;
        mouse = Mouse.GetState();

        Mouse.SetPosition(screen_middle.X, screen_middle.Y); // lock mouse to screen

        if (GamePad.GetState(PlayerIndex.One).Buttons.Back == ButtonState.Pressed || keyboard.IsKeyDown(Keys.Escape))
        { Exit(); }

        if(keyboard.IsKeyDown(Keys.W))
        {
            camera.Move(Vector3.Forward * dt);
        }
        if(keyboard.IsKeyDown(Keys.S))
        {
            camera.Move(Vector3.Backward * dt);
        }

        if(keyboard.IsKeyDown(Keys.A))
        {
            camera.Move(Vector3.Left * dt);
        }
        if(keyboard.IsKeyDown(Keys.D))
        {
            camera.Move(Vector3.Right * dt);
        }

        if(keyboard.IsKeyDown(Keys.Q))
        {
            camera.Move(Vector3.Down * dt);
        }
        if(keyboard.IsKeyDown(Keys.E))
        {
            camera.Move(Vector3.Up * dt);
        }

        if(keyboard.IsKeyDown(Keys.LeftShift))
        {
            camera.speed = 6f;
        }
        else
        {
            camera.speed = 2f;
        }

        camera.Rotate((screen_middle.X - mouse.X) * dt * 0.2f, (screen_middle.Y - mouse.Y) * dt * 0.2f);

        current_level.update(dt, keyboard, last_keyboard, camera);

        // TODO: Add your update logic here

        base.Update(gameTime);
    }

    Color background_color = new Color(0.3f,0.3f,0.3f);
    protected override void Draw(GameTime gameTime)
    {
        double frame_rate = 1.0 / gameTime.ElapsedGameTime.TotalSeconds;
        GraphicsDevice.Clear(background_color);
        GraphicsDevice.DepthStencilState = DepthStencilState.Default;

        _spriteBatch.Begin(SpriteSortMode.BackToFront, BlendState.AlphaBlend);


        basicEffect.World = Matrix.Identity;

        current_level.draw(basicEffect, GraphicsDevice, _spriteBatch, camera);

        basicEffect.World = Matrix.Identity;
        foreach (EffectPass pass in basicEffect.CurrentTechnique.Passes)
        {
            pass.Apply();

            GraphicsDevice.DrawUserPrimitives(PrimitiveType.LineList, origin_axii_lines, 0, 3);
        }
        
        _spriteBatch.DrawString(font, frame_rate.ToString(), Vector2.One, Color.WhiteSmoke, 0, Vector2.Zero, 0.3f, SpriteEffects.None, 0f);

        _spriteBatch.DrawString(font, camera.position.ToString(), new Vector2(screen_x - (camera.position.ToString().Length * 13),1), Color.WhiteSmoke, 0, Vector2.Zero, 0.3f, SpriteEffects.None, 0f);

        _spriteBatch.End();
        base.Draw(gameTime);
    }

    protected override void EndRun()
    {
        current_level.close();

        base.EndRun();
    }
}
