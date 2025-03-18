using System;

using Microsoft.Xna.Framework.Content;
using Microsoft.Xna.Framework.Graphics;
using Microsoft.Xna.Framework.Input;

using Cameras;
using Objects;
using Messengers;
using Microsoft.Xna.Framework;
using MonoGame.Extended.BitmapFonts;

namespace Levels;

public class NetworkedSimulation : ILevel
{
    Simulation sim;
    Messenger<Vector3> velocity_messenger;
    Messenger<float> density_messenger;

    BitmapFont font;


    public NetworkedSimulation(GraphicsDevice graphics_device)
    {
        sim = new Simulation(graphics_device);
    }

    public void draw(BasicEffect effect, GraphicsDevice graphics_device, SpriteBatch sprite_batch, Camera camera)
    {
        sim.Draw(effect);

        sprite_batch.DrawString(font, sim.velocities[0].ToString(), new Vector2(1, 30), Color.WhiteSmoke, 0, Vector2.Zero, 0.3f, SpriteEffects.None, 0f);

        sprite_batch.DrawString(font, "sim dims: (" + sim.width + ", " + sim.height + ", " + sim.depth + ")", new Vector2(1, 60), Color.WhiteSmoke, 0, Vector2.Zero, 0.3f, SpriteEffects.None, 0f);
    }

    public string getName()
    {
        return "Networked Simulation";
    }

    public void init()
    {
        Console.WriteLine("Starting network data messengers");

        // velocity
        velocity_messenger = new Messenger<Vector3>(4000, 
        arr => {
            Vector3[] temp = new Vector3[arr.Length/16];
            for (int i = 0; i < arr.Length; i += 16)
            {
                temp[i/16].X = BitConverter.ToSingle(arr, i);
                temp[i/16].Y = BitConverter.ToSingle(arr, i + 4);
                temp[i/16].Z = BitConverter.ToSingle(arr, i + 8);
            }
            return temp;
        },
        (arr, width, height, depth) => {
            sim.SetVelocity(arr, width, height, depth);
        });
        velocity_messenger.connect();

        // density
        density_messenger = new Messenger<float>(4001,
            arr => {
                float[] temp = new float[arr.Length / 4];
                for (int i = 0; i < arr.Length; i += 4)
                {
                    temp[i / 4] = BitConverter.ToSingle(arr, i);
                }
                return temp;
        },
        (arr, width, height, depth) => {
            sim.SetDensity(arr, width, height, depth);
        });
        density_messenger.connect();

        Console.WriteLine("Started network data messenger");
    }

    public void load_content(ContentManager content, GraphicsDevice graphics_device)
    {
        this.font = MonoGame.Extended.BitmapFonts.BitmapFont.FromFile(graphics_device, "Content/sans-serif.fnt");   
    }

    public void update(float dt, KeyboardState keyboard_state, KeyboardState last_keyboard_state, Camera camera)
    {
        if(velocity_messenger.connected) 
        {
            velocity_messenger.read(); 
        }
        else
        { 
            velocity_messenger.connect(); 
        }

        if(density_messenger.connected) 
        {
            density_messenger.read(); 
        }
        else
        { 
            density_messenger.connect(); 
        }
    }

    public void close()
    {
        if(velocity_messenger.connected)
        {
            velocity_messenger.close();
        }

        if(density_messenger.connected)
        {
            density_messenger.close();
        }
    }
}