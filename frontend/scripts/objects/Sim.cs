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

    public int width;
    public int height;
    public int depth;

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
            colors[i] = new Color( this.cells[i].W, this.cells[i].W, this.cells[i].W);
            positions[i] = this.Get3DPositionVec(i);
            //Console.WriteLine(i + ": " + colors[i] + " pos: " + positions[i] + " sent pos: " + this.cells[i].X + ", " + this.cells[i].Y + ", " + this.cells[i].Z + ", " + this.cells[i].W);
        }

        this.voxelObject.SetData(positions, colors);
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
}