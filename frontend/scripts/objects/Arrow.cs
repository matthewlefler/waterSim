using System;
using System.ComponentModel.DataAnnotations;
using Microsoft.VisualBasic;
using Microsoft.Xna.Framework;
using Microsoft.Xna.Framework.Graphics;

namespace Objects;

public class Arrow : Object 
{
    public float magnitude;
    public Color color;

    private VertexBuffer vertex_buffer;

    private Matrix world_matix;

    private VertexPositionColor[] vertices = new VertexPositionColor[5];

    public Arrow(Vector3 position, Vector3 direction, Color color, GraphicsDevice graphics_device) 
    : base(position, 
           new Quaternion(Vector3.Cross(Vector3.Right, Vector3.Normalize(direction)), Vector3.Dot(Vector3.Right, Vector3.Normalize(direction))),
           graphics_device)
    {
        this.magnitude = direction.Length();
        this.color = color;

        this.vertex_buffer = new VertexBuffer(this.graphics_device, VertexPositionColor.VertexDeclaration, this.vertices.Length, BufferUsage.WriteOnly);

        updateVerts();
    }

    public void setMagnitude(float magnitude) 
    {
        this.magnitude = magnitude;

        updateVerts();
    }

    public void SetPosition(Vector3 position)
    {
        this.position = position;
        this.world_matix = Matrix.CreateTranslation(position);
    }

    public void setDirection(Vector3 direction) 
    {
        this.magnitude = direction.Length();

        this.rotation = new Quaternion(Vector3.Cross(Vector3.Right, Vector3.Normalize(direction)), Vector3.Dot(Vector3.Right, Vector3.Normalize(direction)));

        updateVerts();
    }

    public void setDirection(Vector3 direction, Color color) 
    {
        this.color = color;

        this.magnitude = direction.Length();

        this.rotation = new Quaternion(Vector3.Cross(Vector3.Right, Vector3.Normalize(direction)), Vector3.Dot(Vector3.Right, Vector3.Normalize(direction)));

        updateVerts();
    }

    public void setDirection(Quaternion rotation) 
    {
        this.rotation = rotation;

        updateVerts();
    }

    public void setColor(Color color) 
    {
        this.color = color;
    }
    
    /// <summary>
    /// updates the vertices that are drawn on the screen
    /// </summary>
    private void updateVerts() 
    {
        this.world_matix = Matrix.CreateTranslation(this.position);

        Vector3 point = Vector3.Transform(Vector3.Right * magnitude, this.rotation);
        Vector3 offset = Vector3.Transform(Vector3.Backward * magnitude / 10f, this.rotation);

        vertices[0] = new VertexPositionColor(Vector3.Zero, this.color);
        vertices[1] = new VertexPositionColor(point, this.color);
        vertices[2] = new VertexPositionColor(point * 0.9f - offset, this.color);
        vertices[3] = new VertexPositionColor(point, this.color);
        vertices[4] = new VertexPositionColor(point * 0.9f + offset, this.color);

        this.vertex_buffer.SetData<VertexPositionColor>(this.vertices);
    }

    public override void Draw(BasicEffect effect) 
    {
        graphics_device.SetVertexBuffer(vertex_buffer);
        effect.World = Matrix.CreateTranslation(this.position);
        
        foreach(EffectPass pass in effect.CurrentTechnique.Passes) 
        {
            pass.Apply();
            graphics_device.DrawPrimitives(PrimitiveType.LineStrip, 0, 4);
        }
    }
}

public class ArrowCollection : Object
{
    private int width;
    private int height;
    private int depth;

    private Arrow[] arrows;


    private static Color start = Color.Blue;
    private static Color end = Color.Red;


    public ArrowCollection(int width, int height, int depth, GraphicsDevice graphics_device) : base(Vector3.Zero, Quaternion.Identity, graphics_device)
    {
        this.width = width;
        this.height = height;
        this.depth = depth;

        arrows = new Arrow[width * height * depth];

        for (int i = 0; i < arrows.Length; i++)
        {
            Vector3 position = new Vector3(i % width, i / width % height, i / (width * height));
            arrows[i] = new Arrow(position, Vector3.UnitX, Color.White, graphics_device);
        }
    }

    public void setData(Vector3[] directions) 
    {
        float max_magnitude = 0.0f;
        foreach(Vector3 dir in directions) 
        {
            float len_squared = dir.LengthSquared();
            if(len_squared > max_magnitude)
            {
                max_magnitude = len_squared;
            }
        }
        max_magnitude = MathF.Sqrt(max_magnitude);

        for (int i = 0; i < directions.Length; i++)
        {
            float magnitude = directions[i].Length();
            Vector3 direction;
            if(magnitude > 0.8f)
            {
                direction = Vector3.Normalize(directions[i]) * 0.8f;
            }
            else 
            {
                direction = directions[i];
            }

            Color color = Color.Lerp(start, end, MathF.Min(magnitude / max_magnitude, 1.0f));

            if(magnitude <= 0.001) { color = Color.Black; direction = Vector3.Zero; }
            
            arrows[i].setDirection(direction, color);
        }
    }

    public override void Draw(BasicEffect effect)
    {
        foreach(Arrow arrow in arrows)
        {
            arrow.Draw(effect);
        }
    }
}