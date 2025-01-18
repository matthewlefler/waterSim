using Microsoft.Xna.Framework;
using Microsoft.Xna.Framework.Graphics;
using Microsoft.Xna.Framework.Input;

using Messengers;
using System;
using Objects;
using MonoGame.Extended.BitmapFonts;
using Cameras;
using MonoGame.Extended.Particles;
using MonoGame.Extended.Screens;
using MonoGame.Extended.Input;

namespace frontend;

public class Game1 : Game
{
    private GraphicsDeviceManager _graphics;
    private SpriteBatch _spriteBatch;

    private int screen_x;
    private int screen_y;
    Point screen_middle;

    Texture2D tex;
    BitmapFont font;

    SimpleCamera camera;

    private Messenger messenger;

    private Simulation sim;

    public Game1()
    {
        _graphics = new GraphicsDeviceManager(this);
        Content.RootDirectory = "Content";
        IsMouseVisible = false;
    }

    protected override void Initialize()
    {
        _graphics.PreferredBackBufferHeight = 1300;
        _graphics.PreferredBackBufferWidth = 1300;
        _graphics.ApplyChanges();

        DepthStencilState depthStencilState = new DepthStencilState();
        depthStencilState.DepthBufferEnable = true;
        depthStencilState.DepthBufferFunction = CompareFunction.Greater;
        GraphicsDevice.DepthStencilState = depthStencilState;
        
        screen_x = _graphics.PreferredBackBufferWidth;
        screen_y = _graphics.PreferredBackBufferHeight;
    
        screen_middle = new Point(screen_x / 2, screen_y / 2);

        Console.WriteLine("screen width: " + screen_x + " screen height: " + screen_y);

        // TODO: Add your initialization logic here

        Console.WriteLine("Initalizing simulation object");
        sim= new Simulation(GraphicsDevice);
        Console.WriteLine("Done");

        Console.WriteLine("Starting network data messenger");

        messenger = new Messenger(4331);
        messenger.connect();

        Console.WriteLine("Started network data messenger");

        camera = new SimpleCamera(Vector3.Backward * 2);

        base.Initialize();

        Console.WriteLine("Initialization done");
    }

    protected override void LoadContent()
    {
        _spriteBatch = new SpriteBatch(GraphicsDevice);

        // TODO: use this.Content to load your game content here
        tex = Content.Load<Texture2D>("white");
        font = MonoGame.Extended.BitmapFonts.BitmapFont.FromFile(GraphicsDevice, "Content/sans-serif.fnt");
    }


    float time = 0;
    float dt = 0;

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

        if (GamePad.GetState(PlayerIndex.One).Buttons.Back == ButtonState.Pressed || keyboard.IsKeyDown(Keys.Escape))
            Exit();

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

        camera.Rotate((last_mouse.X - mouse.X) * dt * 0.3f, (last_mouse.Y - mouse.Y) * dt * 0.3f);


        if(messenger.connected) 
        {
            messenger.read(sim); 
        }
        else 
        { 
            messenger.connect(); 
        }

        // TODO: Add your update logic here

        base.Update(gameTime);
    }

    Color background_color = new Color(0.3f,0.3f,0.3f);
    protected override void Draw(GameTime gameTime)
    {
        double frame_rate = 1.0 / gameTime.ElapsedGameTime.TotalSeconds;
        GraphicsDevice.Clear(background_color);

        _spriteBatch.Begin();

        BasicEffect basicEffect = new BasicEffect(GraphicsDevice); //effect
        basicEffect.VertexColorEnabled = true;
        basicEffect.AmbientLightColor = new Vector3(0.5f,0.5f,0.5f);
        basicEffect.EmissiveColor = new Vector3(0.5f, 0.5f, 0.5f);

        basicEffect.View = camera.view_matrix; //camera projections
        basicEffect.Projection = camera.projection_matrix;

        VertexPositionColor[] lines = new VertexPositionColor[]
        {
            new VertexPositionColor(Vector3.Up, Color.Red),
            new VertexPositionColor(Vector3.Zero, Color.Red),
            
            new VertexPositionColor(Vector3.Right, Color.Green),
            new VertexPositionColor(Vector3.Zero, Color.Green),
            
            new VertexPositionColor(Vector3.Forward, Color.Blue),
            new VertexPositionColor(Vector3.Zero, Color.Blue),
        };

        foreach (EffectPass pass in basicEffect.CurrentTechnique.Passes)
        {
            pass.Apply();

            GraphicsDevice.DrawUserPrimitives(PrimitiveType.LineList, lines, 0, 3);
        }
        
        if(sim.voxelObject is not null & sim.voxelObject.vertices.Length > 0)
        {
            sim.voxelObject.Draw(basicEffect);
        }

        _spriteBatch.DrawString(font, frame_rate.ToString(), Vector2.One, Color.WhiteSmoke, 0, Vector2.Zero, 0.3f, SpriteEffects.None, 0f);
        _spriteBatch.DrawString(font, camera.position.ToString(), Vector2.UnitY * 30, Color.WhiteSmoke, 0, Vector2.Zero, 0.3f, SpriteEffects.None, 0f);
        _spriteBatch.DrawString(font, camera.rotation.ToString(), Vector2.UnitY * 60, Color.WhiteSmoke, 0, Vector2.Zero, 0.3f, SpriteEffects.None, 0f);

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
