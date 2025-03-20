using System.IO;
using System.Collections.Generic;

using Microsoft.Xna.Framework.Content;
using Microsoft.Xna.Framework.Graphics;
using Microsoft.Xna.Framework.Input;
using Microsoft.Xna.Framework;

using MonoGame.Extended.BitmapFonts;

using Objects;
using Cameras;

namespace Levels;
public class ReadFileLevel3D : ILevel
{
    List<int[]> changeable_frames;
    List<float[]> density_frames;
    List<Vector3[][]> velocity_frames;

    public int number_of_frames;
    private int current_frame;

    private int simulation_width;
    private int simulation_height;
    private int simulation_depth;

    private int node_count;

    private string filepath;

    private Simulation sim;

    BitmapFont font;

    float frame_timer = 0.0f;


    public ReadFileLevel3D(string filepath, GraphicsDevice graphics_device) 
    {
        this.filepath = filepath;
        this.sim = new Simulation(graphics_device);
        this.number_of_frames = 0;

        this.changeable_frames = new List<int[]>();
        this.density_frames = new List<float[]>();
        this.velocity_frames = new List<Vector3[][]>();
    }

    public void draw(BasicEffect effect, GraphicsDevice graphics_device, SpriteBatch sprite_batch, Camera camera)
    {
        effect.Projection = camera.projection_matrix;
        effect.View = camera.view_matrix;

        sim.Draw(effect);

        sprite_batch.DrawString(font, "current frame = " + current_frame.ToString(), new Vector2(10, 130), Color.White, 0, Vector2.Zero, 0.3f, SpriteEffects.None, 0f);
    }

    public string getName()
    {
        return "Read Premade File (3D)";
    }

    public void init()
    {
        // read file
        string[] lines = File.ReadAllLines(filepath);
        this.number_of_frames = lines.Length;

        string[] dims = lines[0].Trim().Split(" ");

        this.simulation_width  = int.Parse(dims[0]);
        this.simulation_height = int.Parse(dims[1]);
        this.simulation_depth  = int.Parse(dims[2]);

        this.node_count = simulation_width * simulation_height * simulation_depth;

        for (int i = 1; i < lines.Length; i++)
        {
            int[] temp_change = new int[this.node_count];
            float[] temp_dens = new float[this.node_count];
            Vector3[][] temp_vec = new Vector3[27][];
            for (int j = 0; j < 27; j++)
            {
                temp_vec[j] = new Vector3[this.node_count];
            }

            string line = lines[i];
            string[] items = line.Trim().Split(" ");

            for (int j = 0; j < node_count; j++)
            {
                temp_change[j] = int.Parse(items[j * 29]); // node changeability
                temp_dens[j] = float.Parse(items[j * 29 + 1]); // density

                for(int k = 2; k < 29; k++)
                {
                    temp_vec[k - 2][j] = float.Parse(items[j * 29 + k]) * Simulation.possible_velocities[k - 2];
                }           
            } 

            this.changeable_frames.Add(temp_change);
            this.density_frames.Add(temp_dens);
            this.velocity_frames.Add(temp_vec);
        }

        // and setup init frame of the simulation
        this.current_frame = 0;
        this.number_of_frames = density_frames.Count;

        this.sim.SetDensity(density_frames[current_frame], changeable_frames[current_frame], simulation_width, simulation_height, simulation_depth);
        this.sim.SetVelocity(velocity_frames[current_frame], simulation_width, simulation_height, simulation_depth);
    }

    public void load_content(ContentManager content, GraphicsDevice graphics_device)
    {
        this.font = MonoGame.Extended.BitmapFonts.BitmapFont.FromFile(graphics_device, "Content/sans-serif.fnt");
    }

    public void update(float dt, KeyboardState keyboard_state, KeyboardState last_keyboard_state, Camera camera)
    {
        if(keyboard_state.IsKeyDown(Keys.Right) && last_keyboard_state.IsKeyUp(Keys.Right))
        {
            current_frame++;
            if(current_frame > number_of_frames - 1) {
                current_frame = number_of_frames - 1;
            }
            else
            {
                sim.SetDensity(density_frames[current_frame], changeable_frames[current_frame], simulation_width, simulation_height, simulation_depth);
                sim.SetVelocity(velocity_frames[current_frame], simulation_width, simulation_height, simulation_depth);
            }
        }

        if(keyboard_state.IsKeyDown(Keys.Left) && last_keyboard_state.IsKeyUp(Keys.Left))
        {
            current_frame--;
            if(current_frame < 0) {
                current_frame = 0;
            }
            else
            {
                sim.SetDensity(density_frames[current_frame], changeable_frames[current_frame], simulation_width, simulation_height, simulation_depth);
                sim.SetVelocity(velocity_frames[current_frame], simulation_width, simulation_height, simulation_depth);
            }
        }

        if(keyboard_state.IsKeyDown(Keys.Space))
        {
            frame_timer += dt;
            if(frame_timer > 0.05f)
            {
                frame_timer -= 0.05f;
                current_frame++;
                
                // loops
                if(current_frame > number_of_frames - 1) {
                    current_frame = 0;
                }

                sim.SetDensity(density_frames[current_frame], changeable_frames[current_frame], simulation_width, simulation_height, simulation_depth);
                sim.SetVelocity(velocity_frames[current_frame], simulation_width, simulation_height, simulation_depth);
            }
        }

        if(keyboard_state.IsKeyDown(Keys.N)) 
        {
            if(current_frame != 0)
            {
                current_frame = 0;

                sim.SetDensity(density_frames[current_frame], changeable_frames[current_frame], simulation_width, simulation_height, simulation_depth);
                sim.SetVelocity(velocity_frames[current_frame], simulation_width, simulation_height, simulation_depth);
            }
        }

        if(keyboard_state.IsKeyDown(Keys.M)) 
        {
            if(current_frame != number_of_frames - 1)
            {
                current_frame = number_of_frames - 1;

                sim.SetDensity(density_frames[current_frame], changeable_frames[current_frame], simulation_width, simulation_height, simulation_depth);
                sim.SetVelocity(velocity_frames[current_frame], simulation_width, simulation_height, simulation_depth);
            }
        }

        // change whether to show the macro arrows of the micro ones
        if(keyboard_state.IsKeyDown(Keys.L) && last_keyboard_state.IsKeyUp(Keys.L))
        {
            this.sim.draw_macro_arrows = !this.sim.draw_macro_arrows;

            sim.SetVelocity(velocity_frames[current_frame], simulation_width, simulation_height, simulation_depth);
        }
    }

    public void close() 
    {
        return;
    }
}