using Microsoft.Xna.Framework;
using Microsoft.Xna.Framework.Graphics;
using Microsoft.Xna.Framework.Input;

using Messengers;
using System;
using Objects;
using MonoGame.Extended.BitmapFonts;
using Cameras;

namespace frontend;

public class Game1 : Game
{
    private GraphicsDeviceManager _graphics;
    private SpriteBatch _spriteBatch;

    private int screen_x;
    private int screen_y;

    Texture2D tex;
    BitmapFont font;

    SimpleCamera camera;

    private Messenger messenger;

    private Simulation sim = new Simulation();
    private Vector4[,,] sim_data;

    public Game1()
    {
        _graphics = new GraphicsDeviceManager(this);
        Content.RootDirectory = "Content";
        IsMouseVisible = true;
    }

    protected override void Initialize()
    {
        _graphics.PreferredBackBufferHeight = 1300;
        _graphics.PreferredBackBufferWidth = 1300;
        _graphics.ApplyChanges();
        
        screen_x = _graphics.PreferredBackBufferWidth;
        screen_y = _graphics.PreferredBackBufferHeight;
        Console.WriteLine("screen width: " + screen_x + " screen height: " + screen_y);

        // TODO: Add your initialization logic here
        Console.WriteLine("starting network data messenger");

        messenger = new Messenger(4331);
        messenger.connect();

        Console.WriteLine("started network data messenger");

        base.Initialize();
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
    KeyboardState keyboard;

    protected override void Update(GameTime gameTime)
    {
        keyboard = Keyboard.GetState();
        last_mouse = mouse;
        mouse = Mouse.GetState();

        if (GamePad.GetState(PlayerIndex.One).Buttons.Back == ButtonState.Pressed || keyboard.IsKeyDown(Keys.Escape))
            Exit();

        if(keyboard.IsKeyDown(Keys.W))
        {
            camera.Move(Vector3.Forward);
        }
        if(keyboard.IsKeyDown(Keys.S))
        {
            camera.Move(Vector3.Backward);
        }

        if(keyboard.IsKeyDown(Keys.A))
        {
            camera.Move(Vector3.Left);
        }
        if(keyboard.IsKeyDown(Keys.D))
        {
            camera.Move(Vector3.Right);
        }

        Quaternion camera_delta_rotation = Quaternion.CreateFromAxisAngle(Vector3.Up, last_mouse.X - mouse.X);
        camera_delta_rotation += Quaternion.CreateFromAxisAngle(Vector3.Transform(Vector3.Left, camera.rotation), last_mouse.Y - mouse.Y);

        camera.Rotate(camera_delta_rotation);

        dt = (float) gameTime.ElapsedGameTime.TotalSeconds;

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

    protected override void Draw(GameTime gameTime)
    {
        double frame_rate = 1.0 / gameTime.ElapsedGameTime.TotalSeconds;
        GraphicsDevice.Clear(Color.CornflowerBlue);

        Console.WriteLine(sim.voxelObject.vertices.Length);
        
        if(sim.voxelObject is not null & sim.voxelObject.vertices.Length > 0)
        {
            VertexBuffer vertices_buffer = new VertexBuffer(GraphicsDevice, VertexPositionColor.VertexDeclaration, sim.voxelObject.vertices.Length, BufferUsage.WriteOnly);
            vertices_buffer.SetData<VertexPositionColor>(sim.voxelObject.vertices);
            GraphicsDevice.SetVertexBuffer(vertices_buffer);

            GraphicsDevice.DrawIndexedPrimitives(PrimitiveType.TriangleList, 0, 0, sim.voxelObject.vertices.Length / 3);
        }

        _spriteBatch.Begin();

        _spriteBatch.DrawString(font, frame_rate.ToString(), Vector2.One, Color.WhiteSmoke);

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
