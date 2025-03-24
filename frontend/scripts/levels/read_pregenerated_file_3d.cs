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

    public int frame_count;
    private int current_frame;

    private Dictionary<Keys, int> numpad_key_to_frame_number;

    private int simulation_width;
    private int simulation_height;
    private int simulation_depth;

    private int node_count;

    private int current_node = 0;

    private string filepath;

    private Simulation sim;

    BitmapFont font;

    float frame_timer = 0.0f;


    public ReadFileLevel3D(string filepath, GraphicsDevice graphics_device) 
    {
        this.filepath = filepath;
        this.sim = new Simulation(graphics_device);
        this.frame_count = 0;

        this.changeable_frames = new List<int[]>();
        this.density_frames = new List<float[]>();
        this.velocity_frames = new List<Vector3[][]>();

        this.numpad_key_to_frame_number = new Dictionary<Keys, int>();
    }

    public void draw(BasicEffect effect, GraphicsDevice graphics_device, SpriteBatch sprite_batch, Camera camera)
    {
        effect.Projection = camera.projection_matrix;
        effect.View = camera.view_matrix;

        sim.Draw(effect);

        VoxelObject v = new VoxelObject(Vector3.Zero, Quaternion.Identity, [new Vector3(current_node % simulation_width, current_node / simulation_width % simulation_height, current_node / (simulation_width * simulation_height))], [Color.BurlyWood], 0.3f, graphics_device);
        v.Draw(effect);

        sprite_batch.DrawString(font, "current frame = " + current_frame.ToString(), new Vector2(10, 130), Color.White, 0, Vector2.Zero, 0.3f, SpriteEffects.None, 0f);

        sprite_batch.DrawString(font, "current node: " + current_node, new Vector2(10, 150), Color.White, 0, Vector2.Zero, 0.3f, SpriteEffects.None, 0f);

        float density = 0.0f;
        for (int i = 0; i < 27; i++)
        {
            Vector3 vec = velocity_frames[current_frame][i][current_node];

            density += vec.Length();

            sprite_batch.DrawString(font, vec.ToString(), new Vector2(10, 170 + i * 20), Color.White, 0, Vector2.Zero, 0.2f, SpriteEffects.None, 0f);
        }
        sprite_batch.DrawString(font, density.ToString(), new Vector2(10, 170 + 27 * 20), Color.White, 0, Vector2.Zero, 0.2f, SpriteEffects.None, 0f);

        sprite_batch.DrawString(font, sim.densities[current_node].ToString(), new Vector2(10, 170 + 28 * 20), Color.White, 0, Vector2.Zero, 0.2f, SpriteEffects.None, 0f);

        for (int i = 0; i < 27; i++)
        {
            Vector3 vec = new Vector3(current_node % simulation_width, current_node / simulation_width % simulation_height, current_node / (simulation_width * simulation_height));
            vec = vec - Simulation.possible_velocities[i];

            vec.X = (int)vec.X;
            vec.Y = (int)vec.Y;
            vec.Z = (int)vec.Z;

            vec.X = vec.X < 0 ? simulation_width - 1 : vec.X;
            vec.Y = vec.Y < 0 ? simulation_height - 1 : vec.Y;
            vec.Z = vec.Z < 0 ? simulation_depth - 1 : vec.Z;

            vec.X = vec.X > simulation_width - 1 ? 0 : vec.X;
            vec.Y = vec.Y > simulation_height - 1 ? 0 : vec.Y;
            vec.Z = vec.Z > simulation_depth - 1 ? 0 : vec.Z;

            vec.X = (int)vec.X;
            vec.Y = (int)vec.Y;
            vec.Z = (int)vec.Z;

            sprite_batch.DrawString(font, vec.ToString(), new Vector2(220, 170 + i * 20), Color.White, 0, Vector2.Zero, 0.2f, SpriteEffects.None, 0f);
        }

    }

    public string getName()
    {
        return "Read Premade File (3D)";
    }

    public void init()
    {
        // read file
        string[] lines = File.ReadAllLines(filepath);
        this.frame_count = lines.Length;

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

                // vector scalars
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
        this.frame_count = density_frames.Count;

        numpad_key_to_frame_number.Add(Keys.NumPad1, (int)(frame_count * 0.1f));
        numpad_key_to_frame_number.Add(Keys.NumPad2, (int)(frame_count * 0.2f));
        numpad_key_to_frame_number.Add(Keys.NumPad3, (int)(frame_count * 0.3f));
        numpad_key_to_frame_number.Add(Keys.NumPad4, (int)(frame_count * 0.4f));
        numpad_key_to_frame_number.Add(Keys.NumPad5, (int)(frame_count * 0.5f));
        numpad_key_to_frame_number.Add(Keys.NumPad6, (int)(frame_count * 0.6f));
        numpad_key_to_frame_number.Add(Keys.NumPad7, (int)(frame_count * 0.7f));
        numpad_key_to_frame_number.Add(Keys.NumPad8, (int)(frame_count * 0.8f));
        numpad_key_to_frame_number.Add(Keys.NumPad9, (int)(frame_count * 0.9f));
        numpad_key_to_frame_number.Add(Keys.NumPad0, (int)(frame_count * 1.0f) - 1);

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
            if(current_frame > frame_count - 1) {
                current_frame = frame_count - 1;
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

        // change to frame_count / num key # frame
        foreach (Keys key in numpad_key_to_frame_number.Keys)
        {
            if(keyboard_state.IsKeyDown(key) && last_keyboard_state.IsKeyUp(key))
            {
                this.current_frame = numpad_key_to_frame_number[key];

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
                if(current_frame > frame_count - 1) {
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
            if(current_frame != frame_count - 1)
            {
                current_frame = frame_count - 1;

                sim.SetDensity(density_frames[current_frame], changeable_frames[current_frame], simulation_width, simulation_height, simulation_depth);
                sim.SetVelocity(velocity_frames[current_frame], simulation_width, simulation_height, simulation_depth);
            }
        }

        // change whether to show the density voxel object
        if(keyboard_state.IsKeyDown(Keys.B) && last_keyboard_state.IsKeyUp(Keys.B))
        {
            this.sim.draw_density_voxel_object = !this.sim.draw_density_voxel_object;
        }

        // change whether to show the macro arrows of the micro ones
        if(keyboard_state.IsKeyDown(Keys.V) && last_keyboard_state.IsKeyUp(Keys.V))
        {
            this.sim.draw_macro_arrows = !this.sim.draw_macro_arrows;

            sim.SetVelocity(velocity_frames[current_frame], simulation_width, simulation_height, simulation_depth);
        }

        // keys to move around the view node, node, which displays data in numeric form aabout the currently selected node
        if(keyboard_state.IsKeyDown(Keys.I) && last_keyboard_state.IsKeyUp(Keys.I))
        {
            current_node += simulation_width;    
        }

        if(keyboard_state.IsKeyDown(Keys.J) && last_keyboard_state.IsKeyUp(Keys.J))
        {
            ++current_node;
        }

        if(keyboard_state.IsKeyDown(Keys.K) && last_keyboard_state.IsKeyUp(Keys.K))
        {
            current_node -= simulation_width;    
        }

        if(keyboard_state.IsKeyDown(Keys.L) && last_keyboard_state.IsKeyUp(Keys.L))
        {
            --current_node;
        }

        if(keyboard_state.IsKeyDown(Keys.U) && last_keyboard_state.IsKeyUp(Keys.U))
        {
            current_node += simulation_width * simulation_height;
        }

        if(keyboard_state.IsKeyDown(Keys.O) && last_keyboard_state.IsKeyUp(Keys.O))
        {
            current_node -= simulation_width * simulation_height;
        }
        if(current_node > simulation_width * simulation_height * simulation_depth) { current_node = simulation_width * simulation_height * simulation_depth; }
        if(current_node < 0) { current_node = 0; }
    }

    public void close() 
    {
        return;
    }
}