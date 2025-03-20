using System;
using Microsoft.Xna.Framework;
using Microsoft.Xna.Framework.Graphics;

namespace Objects;

public class Simulation : Object
{
    // x, y, z  y+ is up
    // 27 arrays of flattened 3d arrays
    public Vector3[][] micro_velocities;
    public Vector3[] macro_velocities;

    public float[] densities; 

    private static Color density_color_start = Color.Black;
    private static Color density_color_end = Color.WhiteSmoke;

    public VoxelObject voxel_object;
    public ArrowCollection[] micro_flow_arrows;
    public ArrowCollection macro_flow_arrows;

    public bool draw_macro_arrows = true;

    public int width;
    public int height;
    public int depth;

    public static readonly Vector3[] possible_velocities = {    
        new Vector3( 0,  0,  0), 
        new Vector3( 1,  0,  0), 
        new Vector3( 0,  1,  0), 
        new Vector3( 0,  0,  1), 
        new Vector3(-1,  0,  0), 
        new Vector3( 0, -1,  0), 
        new Vector3( 0,  0, -1), 
        new Vector3( 1,  1,  0), 
        new Vector3(-1,  1,  0), 
        new Vector3( 1, -1,  0), 
        new Vector3(-1, -1,  0), 
        new Vector3( 0,  1,  1), 
        new Vector3( 0, -1,  1), 
        new Vector3( 0,  1, -1), 
        new Vector3( 0, -1, -1), 
        new Vector3( 1,  0,  1), 
        new Vector3(-1,  0,  1), 
        new Vector3( 1,  0, -1), 
        new Vector3(-1,  0, -1), 
        new Vector3( 1,  1,  1), 
        new Vector3( 1,  1, -1), 
        new Vector3( 1, -1,  1), 
        new Vector3( 1, -1, -1), 
        new Vector3(-1,  1,  1), 
        new Vector3(-1,  1, -1), 
        new Vector3(-1, -1,  1), 
        new Vector3(-1, -1, -1)
    };

    public Simulation(GraphicsDevice graphics_device) : this(
        [[Vector3.Zero],[Vector3.Zero],[Vector3.Zero], // 27 one for each of the possible velocities 
         [Vector3.Zero],[Vector3.Zero],[Vector3.Zero],
         [Vector3.Zero],[Vector3.Zero],[Vector3.Zero],

         [Vector3.Zero],[Vector3.Zero],[Vector3.Zero],
         [Vector3.Zero],[Vector3.Zero],[Vector3.Zero],
         [Vector3.Zero],[Vector3.Zero],[Vector3.Zero],
         
         [Vector3.Zero],[Vector3.Zero],[Vector3.Zero],
         [Vector3.Zero],[Vector3.Zero],[Vector3.Zero],
         [Vector3.Zero],[Vector3.Zero],[Vector3.Zero],],
         
        new float[] { 0.0f }, 1, 1, 1, graphics_device) { }

    public Simulation(Vector3[][] velocities, float[] densities, int width, int height, int depth, GraphicsDevice graphics_device) : base(Vector3.One, Quaternion.Identity, graphics_device)
    {
        this.micro_velocities = velocities;
        this.densities = densities;

        this.width = width;
        this.height = height;
        this.depth = depth;

        Color[] colors = new Color[this.micro_velocities.Length];
        Vector3[] positions = new Vector3[this.micro_velocities.Length];

        for (int i = 0; i < this.densities.Length; i++)
        {
            colors[i] = Color.Lerp(density_color_start, density_color_end, densities[i] / 10f);
            positions[i] = this.Get3DPositionVector(i);
        }

        this.voxel_object = new VoxelObject(Vector3.Zero, Quaternion.Identity, positions, colors, 0.05f, graphics_device);

        if(velocities.Length != 27) 
        {
            throw new ArgumentException("new_velocities number of sub arrays is not 27");
        }

        this.micro_flow_arrows = new ArrowCollection[27];
        for (int i = 0; i < micro_flow_arrows.Length; i++)
        {
            this.micro_flow_arrows[i] = new ArrowCollection(width, height, depth, graphics_device);
            this.micro_flow_arrows[i].setData(velocities[i]);
        }

        this.macro_flow_arrows = new ArrowCollection(width, height, depth, graphics_device);

        calc_macro_velocity();  
    }

    public void SetVelocity(Vector3[][] new_velocities, int width, int height, int depth)
    {
        if(new_velocities.Length != 27) 
        {
            throw new ArgumentException("new_velocities number of sub arrays is not 27");
        }

        this.micro_velocities = new_velocities;

        this.width = width;
        this.height = height;
        this.depth = depth;
        
        if(draw_macro_arrows)
        {
            calc_macro_velocity(); 

            this.macro_flow_arrows = new ArrowCollection(width, height, depth, graphics_device);
            this.macro_flow_arrows.setData(macro_velocities);      
        }
        else
        {
            for (int i = 0; i < micro_flow_arrows.Length; i++)
            {
                this.micro_flow_arrows[i] = new ArrowCollection(width, height, depth, graphics_device);
                this.micro_flow_arrows[i].setData(micro_velocities[i]);
            }
        }
    } 

    public void SetDensity(float[] new_densities, int[] changeablility, int width, int height, int depth) 
    {
        this.densities = new_densities; 

        this.width = width;
        this.height = height;
        this.depth = depth;

        Color[] colors = new Color[new_densities.Length];
        Vector3[] positions = new Vector3[new_densities.Length];

        // get maximum density
        float maximum_density = 0.0f;
        foreach(float d in densities)
        {
            if(d > maximum_density)
            {
                maximum_density = d;
            }
        }
        
        // get minimum density
        float minimum_density = maximum_density;
        foreach(float d in densities)
        {
            if(d < minimum_density)
            {
                minimum_density = d;
            }
        }

        for (int i = 0; i < new_densities.Length; i++)
        {
            Color color;

            switch(changeablility[i]) 
            {
                case 0:
                    float t = (new_densities[i] - minimum_density) / maximum_density;

                    static float expLerp(float t)
                    {
                        return t > 1 ? 1 : 1 - MathF.Pow(2, -10*t);
                    }

                    t = expLerp(t);

                    if(t < 0.001f)
                    {
                        color = new Color(0.0f, 0.0f, 0.0f, 0.0f);
                    }
                    else
                    {
                        color = Color.Lerp(density_color_start, density_color_end, t);
                    }
                break;

                case 1:
                    color = Color.Purple;
                break;

                case 2:
                    color = Color.Green;
                break;

                default:
                    color = new Color(0, 0, 0, 0);
                break;
            }

            colors[i] = color;
            positions[i] = this.Get3DPositionVector(i);
        }

        this.voxel_object.SetData(positions, colors);
    }

    private void calc_macro_velocity()
    {
        int node_count = this.width * this.height * this.depth;
        macro_velocities = new Vector3[node_count];
        for (int j = 0; j < node_count; j++)
        {
            Vector3 average = Vector3.Zero;
            for (int i = 0; i < 27; i++)
            {
                average += this.micro_velocities[i][j];
            }
            macro_velocities[j] = average;
        }
    }


    override public void Draw(BasicEffect effect)
    {
        if(voxel_object is null) { return; }

        // this.voxel_object.Draw(effect);

        if(draw_macro_arrows)
        {
            macro_flow_arrows.Draw(effect);
        }
        else
        {
            foreach(ArrowCollection collection in this.micro_flow_arrows)
            {
                collection.Draw(effect);
            }
        }
    }

    public (int x, int y, int z) Get3DPosition(int index)
    {
        return (index % width, index / width % height, index / (width * height));
    } 

    public Vector3 Get3DPositionVector(int index)
    {
        return new Vector3(index % width, index / width % height, index / (width * height));
    }  

    public int GetIndex(int x, int y, int z)
    {
        return x + (y * height) + (z * width * height);
    } 

    public Vector3 getVelocityAtPosition(float x, float y, float z)
    {
        return getVelocityAtLocalPosition(new Vector3(x, y, z));
    }

    /// <summary>
    /// uses trilinear interpolation to find the velocity at an arbitary position inside the simulation,
    /// using local coordantes  
    /// </summary>
    /// <param name="pos"> the position to get the interpolated velocity at, cannot be outside the bounds of the simulation </param>
    /// <returns> the linear interpolated velocity at the position</returns>
    /// <exception cref="System.ArgumentException"></exception>
    /// <exception cref="System.ArgumentOutOfRangeException"></exception>
    public Vector3 getVelocityAtLocalPosition(Vector3 pos)
    {
        if(pos.X is float.NaN || pos.Y is float.NaN || pos.Z is float.NaN)
        {
            throw new System.ArgumentException($"At least one of the components of the passed argument {pos.ToString()} is NaN");
        }

        if(pos.X > width - 1 || pos.X < 0 || pos.Y > height - 1 || pos.Y < 0 || pos.Z > depth - 1 || pos.Z < 0)
        {
            throw new System.ArgumentOutOfRangeException($"the argument {pos.ToString()} is outside of the local dimesions of the simulation");
        }

        int minX = (int) pos.X; 
        int minY = (int) pos.Y;
        int minZ = (int) pos.Z;

        int maxX = (int) (pos.X + 1.0f);
        int maxY = (int) (pos.Y + 1.0f);
        int maxZ = (int) (pos.Z + 1.0f);

        float xDec = pos.X - minX;
        float yDec = pos.Y - minY;
        float zDec = pos.Z - minZ;

        //trilinear interpolation
        Vector3 value = (
        (
        (macro_velocities[GetIndex(minX, minY, minZ)] * (yDec) + macro_velocities[GetIndex(minX, maxY, minZ)] * (1-yDec)) * (xDec) +
        (macro_velocities[GetIndex(maxX, minY, minZ)] * (yDec) + macro_velocities[GetIndex(maxX, maxY, minZ)] * (1-yDec)) * (1-xDec)
        ) 
        * (zDec) +
        (
        (macro_velocities[GetIndex(minX, minY, maxZ)] * (yDec) + macro_velocities[GetIndex(minX, maxY, minZ)] * (1-yDec)) * (xDec) + 
        (macro_velocities[GetIndex(maxX, minY, maxZ)] * (yDec) + macro_velocities[GetIndex(maxX, maxY, minZ)] * (1-yDec)) * (1-xDec)
        ) 
        * (1-zDec)
        );

        return new Vector3(value.X, value.Y, value.Z);
    }
}