using System.Collections.Generic;

using Microsoft.Xna.Framework.Content;
using Microsoft.Xna.Framework.Graphics;
using Microsoft.Xna.Framework.Input;
using Microsoft.Xna.Framework;

using MonoGame.Extended.BitmapFonts;
using Cameras;
using System;

namespace Levels;

public class LevelSelector : ILevel
{
    List<ILevel> levels;
    int selected = 0;

    BitmapFont font;

    int screen_width;
    int screen_height;

    frontend.Game1 instance;

    public LevelSelector(ILevel[] levels, int screen_width, int screen_height, frontend.Game1 instance)
    {
        this.screen_width = screen_width;
        this.screen_height = screen_height;

        this.instance = instance;

        this.levels = new();

        this.levels.AddRange(levels);
    }

    public LevelSelector(int screen_width, int screen_height, frontend.Game1 instance) : this([], screen_width, screen_height, instance) { }

    public void addLevel(ILevel level)
    {
        this.levels.Add(level);
    }

    public void draw(BasicEffect effect, GraphicsDevice graphics_device, SpriteBatch sprite_batch, Camera camera)
    {
        graphics_device.Clear(Color.SkyBlue);

        Color color = Color.White;

        float size = 0.7f;
        float height = font.LineHeight * size;

        sprite_batch.DrawString(font, "^", 
                        new Vector2(screen_width / 2.0f - size * (font.MeasureString("^").Width / 2.0f), -1 * height + ((screen_height / 2) - (levels.Count * 30))),
                        color, 0, Vector2.Zero, size, SpriteEffects.None, 0.0f);

        for (int i = 0; i < levels.Count; i++)
        {

            string level_name = levels[i].getName();

            if(this.selected == i) 
            { 
                level_name = "- " + level_name + " -";
            }

            sprite_batch.DrawString(font, level_name, 
                                    new Vector2(screen_width / 2.0f - size * (font.MeasureString(level_name).Width / 2.0f), i * height + ((screen_height / 2) - (levels.Count * 30))),
                                    color, 0, Vector2.Zero, size, SpriteEffects.None, 0.0f);
        }

        sprite_batch.DrawString(font, "^", 
                new Vector2(screen_width / 2.0f + size * (font.MeasureString("^").Width / 2.0f), (levels.Count + 1) * height + ((screen_height / 2) - (levels.Count * 30))),
                color, MathF.PI, Vector2.Zero, size, SpriteEffects.None, 0.0f);
    }


    public void init()
    {
        this.selected = 0;
    }

    public void load_content(ContentManager content, GraphicsDevice graphics_device)
    {
        this.font = MonoGame.Extended.BitmapFonts.BitmapFont.FromFile(graphics_device, "Content/sans-serif.fnt"); 

        foreach(ILevel level in levels) {
            level.load_content(content, graphics_device);
        }  
    }

    public void update(float dt, KeyboardState keyboard_state, KeyboardState last_keyboard_state, Camera camera)
    {
        if(keyboard_state.IsKeyDown(Keys.Down) && last_keyboard_state.IsKeyUp(Keys.Down))
        {
            ++selected;
            if(selected > levels.Count - 1)
            {
                selected = levels.Count - 1;
            }
        }

        if(keyboard_state.IsKeyDown(Keys.Up) && last_keyboard_state.IsKeyUp(Keys.Up))
        {
            --selected;
            if(selected < 0)
            {
                selected = 0;
            }
        }
        if(keyboard_state.IsKeyDown(Keys.Enter)) 
        {
            levels[selected].init();
            instance.setLevel(levels[selected]);
        }
    }

    public void close() 
    {

    }

    public string getName()
    {
        return "Level Selector";
    }
}