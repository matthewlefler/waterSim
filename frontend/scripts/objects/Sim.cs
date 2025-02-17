using System;
using Microsoft.Xna.Framework;
using Microsoft.Xna.Framework.Graphics;

namespace Objects;

public class Simulation : Object
{
    public Vector4[] cells; // x, y, z   y+ is up
    public VoxelObject voxelObject;

    public LineObject[] streamLines;
    const int steps = 10; // the number of line segments each stream line should have 


    public int width;
    public int height;
    public int depth;

    public Simulation(GraphicsDevice graphics_device) : base(Vector3.One, Quaternion.Identity, graphics_device)
    {
        this.cells = new Vector4[] { new Vector4(0, 0, 0, 0) };

        this.width = 1;
        this.height = 1;
        this.depth = 1;


        Color[] colors = new Color[1];
        Vector3[] positions = new Vector3[1];

        for (int i = 0; i < this.cells.Length; i++)
        {
            colors[i] = new Color(this.cells[i].W, this.cells[i].W, this.cells[i].W);
            positions[i] = this.Get3DPositionVec(i);
        }

        this.voxelObject = new VoxelObject(Vector3.Zero, Quaternion.Identity, positions, colors, 0.1f, graphics_device);

        streamLines = new LineObject[0];
        updateStreamLines();

        this.graphics_device = graphics_device;
    }

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

        streamLines = new LineObject[0];
        updateStreamLines();

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

        for (int i = 0; i < this.cells.Length; i++)
        {
            float colX = 1 - (1 / (1 + MathF.Abs(this.cells[i].X / 10f))); // 1 - 1/(1 + |x / 10|)
            float colY = 1 - (1 / (1 + MathF.Abs(this.cells[i].Y / 10f)));
            float colZ = 1 - (1 / (1 + MathF.Abs(this.cells[i].Z / 10f)));

            colors[i] = new Color( colX, colY, colZ);
            positions[i] = this.Get3DPositionVec(i);
        }

        this.voxelObject.SetData(positions, colors);
        
        updateStreamLines();
    } 


    override public void Draw(Effect effect)
    {
        if(voxelObject is null) { return; }

        this.voxelObject.Draw(effect);

        foreach(LineObject lineObject in streamLines) 
        {
            if(lineObject is null) { continue; }

            lineObject.Draw(effect);
        }
    }

    /// <summary>
    /// update the line object collection, streamlines to match the velocities in the cells
    /// </summary>
    public void updateStreamLines()
    {
        if(height < 3 || depth < 3 || width < 3) { return; }

        streamLines = new LineObject[(height-2) * (depth-2)];

        for (int y = 1; y < height - 1; y++)
        {
            for (int z = 1; z < depth - 1; z++)
            {
                streamLines[(z-1) + ((y-1) * (height-2))] = new LineObject(new Vector3(1, y, z), Quaternion.Identity, graphics_device);
            }
        }

        for(int i = 0; i < streamLines.Length; i++)
        {
            LineObject lineObject = streamLines[i];

            Vector3[] positions = new Vector3[steps];

            Vector3 pos = lineObject.position;
            Vector3 velocity;

            for(int j = 0; j < steps; j++) 
            {
                // position goes outside the sim, stop and copy the current to a new shorter array, and breal the loop
                if(pos.X > width - 1 || pos.X < 0 || pos.Y > height - 1 || pos.Y < 0 || pos.Z > depth - 1 || pos.Z < 0)
                {
                    Vector3[] temp = positions;
                    positions = new Vector3[j];
                    for (int k = 0; k < positions.Length; k++)
                    {
                        positions[k] = temp[k];
                    }

                    break;
                }

                positions[j] = pos;

                velocity = getVelocityAtPosition(pos);
                velocity.Normalize();
                pos += velocity;
            }

            lineObject.SetData(positions);
        }
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
        return getVelocityAtPosition(new Vector3(x, y, z));
    }

    public Vector3 getVelocityAtPosition(Vector3 pos)
    {
        if(pos.X is float.NaN || pos.Y is float.NaN || pos.Z is float.NaN)
        {
            throw new System.ArgumentNullException($"At least one of the components of the argument {pos.ToString()} is NaN");
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