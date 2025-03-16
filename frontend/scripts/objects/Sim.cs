using System;
using Microsoft.Xna.Framework;
using Microsoft.Xna.Framework.Graphics;

namespace Objects;

public class Simulation : Object
{
    public Vector4[] cells; // x, y, z   y+ is up
    public VoxelObject voxelObject;
    public ArrowCollection flowArrows;

    public int width;
    public int height;
    public int depth;

    public Simulation(GraphicsDevice graphics_device) : this(new Vector4[] { Vector4.Zero }, 1, 1, 1, graphics_device) { }

    public Simulation(Vector4[] cells, int width, int height, int depth, GraphicsDevice graphics_device) : base(Vector3.One, Quaternion.Identity, graphics_device)
    {
        this.cells = cells;

        this.width = width;
        this.height = height;
        this.depth = depth;

        Color[] colors = new Color[this.cells.Length];
        Vector3[] positions = new Vector3[this.cells.Length];

        for (int i = 0; i < this.cells.Length; i++)
        {
            colors[i] = new Color(this.cells[i].W, this.cells[i].W, this.cells[i].W);
            positions[i] = this.Get3DPositionVec(i);
        }

        this.voxelObject = new VoxelObject(Vector3.Zero, Quaternion.Identity, positions, colors, 0.1f, graphics_device);
        this.flowArrows = new ArrowCollection(width, height, depth, graphics_device);

        this.graphics_device = graphics_device;
    }

    public void SetData(Vector4[] new_cells, int width, int height, int depth)
    {
        this.cells = new_cells;

        this.width = width;
        this.height = height;
        this.depth = depth;

        Color[] colors = new Color[this.cells.Length];
        Vector3[] positions = new Vector3[this.cells.Length];

        Vector3[] velocities = new Vector3[this.cells.Length];

        for (int i = 0; i < this.cells.Length; i++)
        {
            float colX = 1 - (1 / (1 + MathF.Abs(this.cells[i].X / 10f))); // 1 - 1/(1 + |x / 10|)
            float colY = 1 - (1 / (1 + MathF.Abs(this.cells[i].Y / 10f)));
            float colZ = 1 - (1 / (1 + MathF.Abs(this.cells[i].Z / 10f)));

            colors[i] = new Color( colX, colY, colZ);
            positions[i] = this.Get3DPositionVec(i);

            velocities[i] = new Vector3(new_cells[i].X, new_cells[i].Y, new_cells[i].Z);
        }

        this.voxelObject.SetData(positions, colors);
        this.flowArrows.setData(velocities);        
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

    public Vector3 Get3DPositionVec(int index)
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
        Vector4 value = (
        (
        (cells[GetIndex(minX, minY, minZ)] * (yDec) + cells[GetIndex(minX, maxY, minZ)] * (1-yDec)) * (xDec) +
        (cells[GetIndex(maxX, minY, minZ)] * (yDec) + cells[GetIndex(maxX, maxY, minZ)] * (1-yDec)) * (1-xDec)
        ) 
        * (zDec) +
        (
        (cells[GetIndex(minX, minY, maxZ)] * (yDec) + cells[GetIndex(minX, maxY, minZ)] * (1-yDec)) * (xDec) + 
        (cells[GetIndex(maxX, minY, maxZ)] * (yDec) + cells[GetIndex(maxX, maxY, minZ)] * (1-yDec)) * (1-xDec)
        ) 
        * (1-zDec)
        );

        return new Vector3(value.X, value.Y, value.Z);
    }
}