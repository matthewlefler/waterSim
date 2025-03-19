using System;
using Microsoft.Xna.Framework;
using Microsoft.Xna.Framework.Graphics;

namespace Objects;

public class Simulation : Object
{
    // x, y, z  y+ is up
    // flattened 3d array
    public Vector3[] velocities; 
    public float[] densities; 

    private static Color density_color_start = Color.Black;
    private static Color density_color_end = Color.WhiteSmoke;

    public VoxelObject voxelObject;
    public ArrowCollection flowArrows;

    public int width;
    public int height;
    public int depth;

    public Simulation(GraphicsDevice graphics_device) : this(new Vector3[] { Vector3.Zero }, new float[] { 0.0f }, 1, 1, 1, graphics_device) { }

    public Simulation(Vector3[] velocities, float[] densities, int width, int height, int depth, GraphicsDevice graphics_device) : base(Vector3.One, Quaternion.Identity, graphics_device)
    {
        this.velocities = velocities;
        this.densities = densities;

        this.width = width;
        this.height = height;
        this.depth = depth;

        Color[] colors = new Color[this.velocities.Length];
        Vector3[] positions = new Vector3[this.velocities.Length];

        for (int i = 0; i < this.velocities.Length; i++)
        {
            colors[i] = Color.Lerp(density_color_start, density_color_end, densities[i] / 10f);
            positions[i] = this.Get3DPositionVector(i);
        }

        this.voxelObject = new VoxelObject(Vector3.Zero, Quaternion.Identity, positions, colors, 0.05f, graphics_device);

        this.flowArrows = new ArrowCollection(width, height, depth, graphics_device);
        this.flowArrows.setData(velocities);

        this.graphics_device = graphics_device;
    }

    public void SetVelocity(Vector3[] new_velocities, int width, int height, int depth)
    {
        this.velocities = new_velocities;

        this.width = width;
        this.height = height;
        this.depth = depth;
        
        this.flowArrows = new ArrowCollection(width, height, depth, graphics_device);
        this.flowArrows.setData(new_velocities);        
    } 

    public void SetDensity(float[] new_densities, int width, int height, int depth) 
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
            float t = (new_densities[i] - minimum_density) / maximum_density;

            static float expLerp(float t)
            {
                return t > 1 ? 1 : 1 - MathF.Pow(2, -10*t);
            }

            t = expLerp(t);

            Color color;
            if(t < 0.001f)
            {
                color = new Color(0.0f, 0.0f, 0.0f, 0.0f);
            }
            else
            {
                color = Color.Lerp(density_color_start, density_color_end, t);
            }

            colors[i] = color;
            positions[i] = this.Get3DPositionVector(i);
        }

        this.voxelObject.SetData(positions, colors);
    }


    override public void Draw(BasicEffect effect)
    {
        if(voxelObject is null) { return; }

        this.voxelObject.Draw(effect);
        this.flowArrows.Draw(effect);
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
        (velocities[GetIndex(minX, minY, minZ)] * (yDec) + velocities[GetIndex(minX, maxY, minZ)] * (1-yDec)) * (xDec) +
        (velocities[GetIndex(maxX, minY, minZ)] * (yDec) + velocities[GetIndex(maxX, maxY, minZ)] * (1-yDec)) * (1-xDec)
        ) 
        * (zDec) +
        (
        (velocities[GetIndex(minX, minY, maxZ)] * (yDec) + velocities[GetIndex(minX, maxY, minZ)] * (1-yDec)) * (xDec) + 
        (velocities[GetIndex(maxX, minY, maxZ)] * (yDec) + velocities[GetIndex(maxX, maxY, minZ)] * (1-yDec)) * (1-xDec)
        ) 
        * (1-zDec)
        );

        return new Vector3(value.X, value.Y, value.Z);
    }
}