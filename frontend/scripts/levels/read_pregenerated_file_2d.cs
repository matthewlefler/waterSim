using System.IO;
using System.Collections.Generic;

using Microsoft.Xna.Framework.Content;
using Microsoft.Xna.Framework.Graphics;
using Microsoft.Xna.Framework.Input;
using Microsoft.Xna.Framework;

using MonoGame.Extended.BitmapFonts;

using Objects;
using Cameras;
using System.Data;
using System;

namespace Levels;
public class ReadFileLevel2D : ILevel
{
    List<int[]> changeable_frames;
    List<float[]> density_frames;
    List<Texture2D[]> velocity_frames;

    Texture2D current_texture;

    public int number_of_frames;
    private int current_frame;
    private int current_height = 0;

    private int simulation_width;
    private int simulation_height;
    private int simulation_depth;

    private int node_count;

    private string filepath;

    BitmapFont font;

    float frame_timer = 0.0f;

    GraphicsDevice graphics_device;

    private static Color start = Color.Blue;
    private static Color end = Color.Red;

    private int screen_width;
    private int screen_height;


    public ReadFileLevel2D(string filepath, GraphicsDevice graphics_device, int screen_width, int screen_height) 
    {
        this.filepath = filepath;
        this.number_of_frames = 0;

        this.screen_width = screen_width;
        this.screen_height = screen_height;

        this.graphics_device = graphics_device;

        this.changeable_frames = new List<int[]>();
        this.density_frames = new List<float[]>();
        this.velocity_frames = new List<Texture2D[]>();
    }

    public void draw(BasicEffect effect, GraphicsDevice graphics_device, SpriteBatch sprite_batch, Camera camera)
    {
        effect.Projection = camera.projection_matrix;
        effect.View = camera.view_matrix;

        sprite_batch.Draw(current_texture, new Rectangle(new Point(0, 0), new Point(screen_width, screen_height)), Color.White);

        sprite_batch.DrawString(font, "current frame = " + current_frame.ToString(), new Vector2(10, 130), Color.White, 0, Vector2.Zero, 0.3f, SpriteEffects.None, 0f);
    }

    public string getName()
    {
        return "Read Premade File (2D)";
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
            Color[][] temp_texures_colors = new Color[this.simulation_height][];
            for (int j = 0; j < this.simulation_height; j++)
            {
                temp_texures_colors[j] = new Color[this.simulation_width * this.simulation_depth];
            }

            string line = lines[i];
            string[] items = line.Trim().Split(" ");

            Vector3[] temp_vectors = new Vector3[node_count];
            for (int j = 0; j < node_count; j++)
            {
                temp_change[j] = int.Parse(items[j * 29]); // node changeability
                temp_dens[j] = float.Parse(items[j * 29 + 1]); // density

                Vector3 average = Vector3.Zero;
                for(int k = 2; k < 29; k++)
                {
                    average += float.Parse(items[j * 29 + k]) * Simulation.possible_velocities[k - 2];
                }           
                temp_vectors[j] = average;
            }

            float max_len = 0.0f;
            foreach (Vector3 v in temp_vectors)
            {
                if(v.LengthSquared() > max_len)
                {
                    max_len = v.LengthSquared();
                }
            }
            max_len = MathF.Sqrt(max_len);

            for (int j = 0; j < node_count; j++)
            {
                int y = j / this.simulation_width % this.simulation_height;

                float magnitude = temp_vectors[j].Length();
                Color color;
                if(temp_change[j] == 0)
                {
                    color = Color.Lerp(start, end, MathF.Min(magnitude / max_len, 1.0f));
                }
                else if(temp_change[j] == 1)
                {
                    color = Color.Purple;
                }
                else if(temp_change[j] == 2)
                {
                    color = Color.Green;
                }
                else
                {
                    color = Color.Black;
                }
                temp_texures_colors[y][j] = color;
            }

            Texture2D[] temp_texures = new Texture2D[this.simulation_height];
            
            for(int y = 0; y < this.simulation_height; ++y)
            {
                temp_texures[y] = new Texture2D(graphics_device, simulation_width, simulation_depth);
                temp_texures[y].SetData<Color>(temp_texures_colors[y]);
            }

            this.changeable_frames.Add(temp_change);
            this.density_frames.Add(temp_dens);
            this.velocity_frames.Add(temp_texures);
        }

        // and setup init frame of the simulation
        this.current_height = 0;
        this.current_frame = 0;
        this.number_of_frames = density_frames.Count;

        this.current_texture = this.velocity_frames[current_frame][current_height];
    }

    public void load_content(ContentManager content, GraphicsDevice graphics_device)
    {
        this.font = MonoGame.Extended.BitmapFonts.BitmapFont.FromFile(graphics_device, "Content/sans-serif.fnt");
    }

    public void update(float dt, KeyboardState keyboard_state, KeyboardState last_keyboard_state, Camera camera)
    {
        // next frame
        if(keyboard_state.IsKeyDown(Keys.Right) && last_keyboard_state.IsKeyUp(Keys.Right))
        {
            current_frame++;
            if(current_frame > number_of_frames - 1) {
                current_frame = number_of_frames - 1;
            }
            else
            {
                this.current_texture = velocity_frames[current_frame][current_height];
            }
        }

        // previous frame
        if(keyboard_state.IsKeyDown(Keys.Left) && last_keyboard_state.IsKeyUp(Keys.Left))
        {
            current_frame--;
            if(current_frame < 0) {
                current_frame = 0;
            }
            else
            {
                this.current_texture = velocity_frames[current_frame][current_height];
            }
        }

        // higher y level
        if(keyboard_state.IsKeyDown(Keys.Up) && last_keyboard_state.IsKeyUp(Keys.Up))
        {
            current_height++;
            if(current_height > simulation_height - 1) {
                current_height = simulation_height - 1;
            }
            else
            {
                this.current_texture = velocity_frames[current_frame][current_height];
            }
        }

        // lower y level
        if(keyboard_state.IsKeyDown(Keys.Down) && last_keyboard_state.IsKeyUp(Keys.Down))
        {
            current_height--;
            if(current_height < 0) {
                current_height = 0;
            }
            else
            {
                this.current_texture = velocity_frames[current_frame][current_height];
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

                this.current_texture = velocity_frames[current_frame][current_height];
            }
        }

        if(keyboard_state.IsKeyDown(Keys.N)) 
        {
            if(current_frame != 0)
            {
                current_frame = 0;
                
                this.current_texture = velocity_frames[current_frame][current_height];
            }
        }
        
        if(keyboard_state.IsKeyDown(Keys.M)) 
        {
            if(current_frame != number_of_frames - 1)
            {
                current_frame = number_of_frames - 1;

                this.current_texture = velocity_frames[current_frame][current_height];
            }
        }
    }

    public void close() 
    {
        return;
    }
}