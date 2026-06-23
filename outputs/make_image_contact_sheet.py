from pathlib import Path

from PIL import Image, ImageDraw


src = Path("outputs/图片材料")
out = Path("outputs/图片材料_contact_sheet.jpg")
files = [p for p in src.iterdir() if p.suffix.lower() in [".jpg", ".jpeg", ".png"]]

thumb_w, thumb_h = 240, 160
pad = 24
label_h = 34
cols = 3
rows = (len(files) + cols - 1) // cols
sheet = Image.new("RGB", (cols * (thumb_w + pad) + pad, rows * (thumb_h + label_h + pad) + pad), "white")
draw = ImageDraw.Draw(sheet)

for i, path in enumerate(files):
    im = Image.open(path).convert("RGB")
    im.thumbnail((thumb_w, thumb_h))
    x = pad + (i % cols) * (thumb_w + pad)
    y = pad + (i // cols) * (thumb_h + label_h + pad)
    frame = Image.new("RGB", (thumb_w, thumb_h), (245, 245, 245))
    frame.paste(im, ((thumb_w - im.width) // 2, (thumb_h - im.height) // 2))
    sheet.paste(frame, (x, y))
    draw.text((x, y + thumb_h + 6), path.name, fill=(0, 0, 0))
    print(path.name, Image.open(path).size)

sheet.save(out, quality=92)
print(out.resolve())
