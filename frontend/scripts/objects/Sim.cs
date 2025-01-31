using System;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Security.Cryptography.X509Certificates;
using System.Threading;
using Microsoft.Xna.Framework;
using Microsoft.Xna.Framework.Graphics;

namespace Objects;

public class Simulation
{
    public Vector4[] cells; // x, y, z   y+ is up
    public VoxelObject voxelObject;

    public LineObject[] streamLines;
    int steps = 10; // the number of line segments each stream line should have 


    public int width;
    public int height;
    public int depth;

    GraphicsDevice graphics_device;

    public Simulation(GraphicsDevice graphics_device)
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

        streamLines = new LineObject[width * height];

        for (int x = 0; x < width; x++)
        {
            for (int y = 0; y < height; y++)
            {
                streamLines[x * width + y] = new LineObject(new Vector3(x, y, 0), Quaternion.Identity, graphics_device);
            }
        }

        this.graphics_device = graphics_device;
    }

    public Simulation(Vector4[] cells, int width, int height, int depth, GraphicsDevice graphics_device)
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

        streamLines = new LineObject[width * height];

        for (int x = 0; x < width; x++)
        {
            for (int y = 0; y < height; y++)
            {
                streamLines[x * width + y] = new LineObject(new Vector3(x, y, 0), Quaternion.Identity, graphics_device);
            }
        }

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
            //Console.WriteLine(i + ": " + colors[i] + " pos: " + positions[i] + " sent pos: " + this.cells[i].X + ", " + this.cells[i].Y + ", " + this.cells[i].Z + ", " + this.cells[i].W);
        }

        this.voxelObject.SetData(positions, colors);

        streamLines = new LineObject[width * height];

        for (int x = 0; x < width; x++)
        {
            for (int y = 0; y < height; y++)
            {
                streamLines[x * width + y] = new LineObject(new Vector3(x, y, 0), Quaternion.Identity, graphics_device);
            }
        }
    } 

    public void updateStreamLines() 
    {
        foreach(LineObject lineObject in streamLines)
        {
            Vector3[] positions = new Vector3[steps];
            Vector3 pos = lineObject.position;
            for(int i = 0; i < steps; i++) 
            {
                if(pos.X > width || pos.X < 0 || pos.Y > height || pos.Y < 0 || pos.Z > depth || pos.Z < 0)
                {
                    break;
                }

                positions[i] = pos;

                pos += getVelocityAtPosition(pos);
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
        if(pos.X > width || pos.X < 0 || pos.Y > height || pos.Y < 0 || pos.Z > depth || pos.Z < 0)
        {
            throw new System.ArgumentException("the argument {pos} is outside of the local dimesions of the simulation");
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
        (cells[minX + minY * height + minZ * width * height] * (yDec) + cells[minX + maxY * height + minZ * width * height] * (1-yDec)) * (xDec) +
        (cells[maxX + minY * height + minZ * width * height] * (yDec) + cells[maxX + maxY * height + minZ * width * height] * (1-yDec)) * (1-xDec)
        ) 
        * (zDec) +
        (
        (cells[minX + minY * height + maxZ * width * height] * (yDec) + cells[minX + maxY * height + maxZ * width * height] * (1-yDec)) * (xDec) + 
        (cells[maxX + minY * height + maxZ * width * height] * (yDec) + cells[maxX + maxY * height + maxZ * width * height] * (1-yDec)) * (1-xDec)
        ) 
        * (1-zDec)
        );

        return new Vector3(value.X, value.Y, value.Z);
    }
}