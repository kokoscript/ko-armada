import PIL.Image

img = PIL.Image.open('quilt.png')
pixels = img.load()

width, height = img.size
print(f'{width} x {height}')
print(f'{width * height}px')

with open('imgdata', 'w') as f:
    for y in range(height):
        for x in range(width):
            f.write(f'{pixels[x, y]},')
        f.write('\n')
