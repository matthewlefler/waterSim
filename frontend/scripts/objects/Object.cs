using Microsoft.Xna.Framework;
using Microsoft.Xna.Framework.Graphics;

namespace Objects;

public abstract class Object
{
    public Vector3 position;
    public Quaternion rotation;

    protected GraphicsDevice graphics_device;

    public Object(Vector3 position, Quaternion rotation, GraphicsDevice graphics_device)
    {
        this.position = position;
        this.rotation = rotation;

        this.graphics_device = graphics_device;
    }

    public abstract void Draw(BasicEffect effect);
}

/// <summary>
/// a collection of voxels in any position with a constant size
/// </summary>
public class VoxelObject : Object
{
    public Vector3[] voxel_cube_offsets;
    public float voxel_size;

    public Color[] voxel_colors; // colors of the voxels
    public VertexPositionColor[] vertices;
    private short[] indices;

    private VertexBuffer vertex_buffer;
    private IndexBuffer index_buffer;

    /// <summary>
    /// Creates and returns a voxel object
    /// </summary>
    /// <param name="position"> The initial position of this object </param>
    /// <param name="rotation"> The initial rotation of this object </param>
    /// <param name="voxel_positions"> The array of positions of each voxel </param>
    /// <param name="voxel_colors"> The color of each voxel, the voxel at 3.2e+10index i will have a color at the corresponding index i in this array </param>
    /// <param name="voxel_size"> The size of each voxel </param>
    /// <param name="graphics_device"> The device on which to render the voxels </param>
    /// <exception cref="System.ArgumentException"></exception>
    public VoxelObject(Vector3 position, Quaternion rotation, 
                       Vector3[] voxel_positions, Color[] voxel_colors, float voxel_size,
                       GraphicsDevice graphics_device)
     : base(position, rotation, graphics_device)
    {
        if(voxel_colors.Length != voxel_positions.Length)
        {
            throw new System.ArgumentException("voxel object creation with voxel colors length not matching voxel positions length");
        }

        this.voxel_cube_offsets = voxel_positions;
        vertices = new VertexPositionColor[voxel_cube_offsets.Length * 12 * 3]; // 12 triangles * 3 vertex per triangle 
        this.voxel_colors = voxel_colors;
        this.voxel_size = voxel_size;

        UpdateVertices();
    }

    /// <summary>
    /// sets the vertex and color data and then updates the related data
    /// </summary>
    /// <param name="voxel_positions"></param>
    /// <param name="voxel_colors"></param>
    /// <exception cref="System.ArgumentException"></exception>
    public void SetData(Vector3[] voxel_positions, Color[] voxel_colors)
    {
        if(voxel_colors.Length != voxel_positions.Length)
        {
            throw new System.ArgumentException($"voxel object set data, voxel colors length does not match voxel positions length \n positions length: {voxel_positions.Length} colors length: {voxel_colors.Length}");
        }

        this.voxel_cube_offsets = voxel_positions;
        this.voxel_colors = voxel_colors;

        this.UpdateVertices();
    }

    /// <summary>
    /// recalc the vertices and vertex buffer, the indices and index buffer
    /// can only support up to 8192 voxels which is approx. a 20x20x20 cube
    /// </summary>
    public void UpdateVertices()
    {
        this.vertices = new VertexPositionColor[voxel_cube_offsets.Length * 8]; // 8 vertices per cube 
        this.indices = new short[voxel_cube_offsets.Length * 36];

        Vector3[] cube_points = new Vector3[] {
            (Vector3.Up   + Vector3.Forward  + Vector3.Left ) * voxel_size * 0.5f, // vertex 0
            (Vector3.Up   + Vector3.Forward  + Vector3.Right) * voxel_size * 0.5f, // vertex 1
            (Vector3.Up   + Vector3.Backward + Vector3.Left ) * voxel_size * 0.5f, // vertex 2
            (Vector3.Up   + Vector3.Backward + Vector3.Right) * voxel_size * 0.5f, // vertex 3
            (Vector3.Down + Vector3.Forward  + Vector3.Left ) * voxel_size * 0.5f, // vertex 4
            (Vector3.Down + Vector3.Forward  + Vector3.Right) * voxel_size * 0.5f, // vertex 5
            (Vector3.Down + Vector3.Backward + Vector3.Left ) * voxel_size * 0.5f, // vertex 6
            (Vector3.Down + Vector3.Backward + Vector3.Right) * voxel_size * 0.5f, // vertex 7
        };
        /*
         *     0 ______1
         *     /|     /|
         *    / |    / |
         *  2/__|___/3 |
         *   |  |__|___|
         *   | 4/  |  /5
         *   | /   | /
         *   |/____|/
         *   6      7
        */

        for(int i = 0, j = 0, k = 0; i < this.voxel_cube_offsets.Length; i++, j+=8, k+=36)
        {
            Vector3 voxel_offset = voxel_cube_offsets[i];
            Color voxel_color = voxel_colors[i];

            vertices[j    ] = new VertexPositionColor(voxel_offset + cube_points[0], voxel_color); // vertex 0
            vertices[j + 1] = new VertexPositionColor(voxel_offset + cube_points[1], voxel_color); // vertex 1
            vertices[j + 2] = new VertexPositionColor(voxel_offset + cube_points[2], voxel_color); // vertex 2
            vertices[j + 3] = new VertexPositionColor(voxel_offset + cube_points[3], voxel_color); // vertex 3
            vertices[j + 4] = new VertexPositionColor(voxel_offset + cube_points[4], voxel_color); // vertex 4
            vertices[j + 5] = new VertexPositionColor(voxel_offset + cube_points[5], voxel_color); // vertex 5
            vertices[j + 6] = new VertexPositionColor(voxel_offset + cube_points[6], voxel_color); // vertex 6
            vertices[j + 7] = new VertexPositionColor(voxel_offset + cube_points[7], voxel_color); // vertex 7

            indices[k + 0 ] = (short) (j    ); indices[k + 1 ] = (short) (j + 1); indices[k + 2 ] = (short) (j + 3); // tri 1  top
            indices[k + 3 ] = (short) (j + 2); indices[k + 4 ] = (short) (j    ); indices[k + 5 ] = (short) (j + 3); // tri 2
            indices[k + 6 ] = (short) (j + 2); indices[k + 7 ] = (short) (j + 3); indices[k + 8 ] = (short) (j + 7); // tri 3  front
            indices[k + 9 ] = (short) (j + 6); indices[k + 10] = (short) (j + 2); indices[k + 11] = (short) (j + 7); // tri 4
            indices[k + 12] = (short) (j + 3); indices[k + 13] = (short) (j + 1); indices[k + 14] = (short) (j + 5); // tri 5  right
            indices[k + 15] = (short) (j + 7); indices[k + 16] = (short) (j + 3); indices[k + 17] = (short) (j + 5); // tri 6
            indices[k + 18] = (short) (j + 5); indices[k + 19] = (short) (j + 1); indices[k + 20] = (short) (j    ); // tri 7  back
            indices[k + 21] = (short) (j + 4); indices[k + 22] = (short) (j + 5); indices[k + 23] = (short) (j    ); // tri 8
            indices[k + 24] = (short) (j + 4); indices[k + 25] = (short) (j    ); indices[k + 26] = (short) (j + 2); // tri 9  left
            indices[k + 27] = (short) (j + 4); indices[k + 28] = (short) (j + 2); indices[k + 29] = (short) (j + 6); // tri 10
            indices[k + 30] = (short) (j + 7); indices[k + 31] = (short) (j + 4); indices[k + 32] = (short) (j + 6); // tri 11 bottom
            indices[k + 33] = (short) (j + 7); indices[k + 34] = (short) (j + 5); indices[k + 35] = (short) (j + 4); // tri 12
        }

        this.vertex_buffer = new VertexBuffer(this.graphics_device, VertexPositionColor.VertexDeclaration, this.vertices.Length, BufferUsage.WriteOnly);
        this.vertex_buffer.SetData<VertexPositionColor>(this.vertices);

        this.index_buffer = new IndexBuffer(this.graphics_device, typeof(short), indices.Length, BufferUsage.WriteOnly);
        this.index_buffer.SetData<short>(indices);
    }

    public override void Draw(BasicEffect effect)
    {
        graphics_device.SetVertexBuffer(this.vertex_buffer);
        graphics_device.Indices = index_buffer;

        foreach (EffectPass pass in effect.CurrentTechnique.Passes)
        {
            pass.Apply();
            
            this.graphics_device.DrawIndexedPrimitives(PrimitiveType.TriangleList, 0, 0, this.indices.Length / 3);
        }
    }
}

public class LineObject : Object
{
    private Vector3[] line_points;

    private VertexBuffer vertex_buffer;

    public LineObject(Vector3 position, Quaternion rotation, GraphicsDevice graphics_device) : base(position, rotation, graphics_device)
    {
        line_points = [Vector3.Zero, Vector3.Zero];

        UpdateVertices();
    }

    public void SetData(Vector3[] points) 
    {
        this.line_points = points;

        UpdateVertices();
    }

    public void UpdateVertices()
    {
        if(line_points.Length < 2) { return; }
        
        VertexPositionColor[] vertices = new VertexPositionColor[line_points.Length];

        for (int i = 0; i < vertices.Length; i++)
        {
            vertices[i] = new VertexPositionColor(line_points[i], Color.BlueViolet);
        }

        vertex_buffer = new VertexBuffer(graphics_device, VertexPositionColor.VertexDeclaration, line_points.Length, BufferUsage.WriteOnly);
        vertex_buffer.SetData<VertexPositionColor>(vertices);
    }

    public override void Draw(BasicEffect effect)
    {
        if(line_points.Length < 2) { return; }

        graphics_device.SetVertexBuffer(vertex_buffer);
        effect.World = Matrix.CreateTranslation(this.position);

        foreach(EffectPass pass in effect.CurrentTechnique.Passes) 
        {
            graphics_device.DrawPrimitives(PrimitiveType.LineStrip, 0, line_points.Length - 1);
        }
    }

    public Vector3[] getPositions() 
    {
        return line_points;
    }
}
