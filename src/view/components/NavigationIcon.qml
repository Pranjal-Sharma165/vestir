
import QtQuick

Canvas {
    id: root

    property string iconName: "home"
    property color iconColor: "#274C77"

    width: 24
    height: 24

    onPaint: {
        const ctx = getContext("2d")
        const w = width
        const h = height

        ctx.reset()
        ctx.strokeStyle = iconColor
        ctx.lineWidth = 1.8
        ctx.lineCap = "round"
        ctx.lineJoin = "round"

        ctx.beginPath()

        switch (iconName) {
        case "home":
            ctx.moveTo(w * 0.12, h * 0.46)
            ctx.lineTo(w * 0.5, h * 0.13)
            ctx.lineTo(w * 0.88, h * 0.46)
            ctx.moveTo(w * 0.22, h * 0.40)
            ctx.lineTo(w * 0.22, h * 0.88)
            ctx.lineTo(w * 0.78, h * 0.88)
            ctx.lineTo(w * 0.78, h * 0.40)
            ctx.moveTo(w * 0.43, h * 0.88)
            ctx.lineTo(w * 0.43, h * 0.62)
            ctx.lineTo(w * 0.57, h * 0.62)
            ctx.lineTo(w * 0.57, h * 0.88)
            break

        case "outfits":
            ctx.moveTo(w * 0.35, h * 0.18)
            ctx.lineTo(w * 0.22, h * 0.25)
            ctx.lineTo(w * 0.08, h * 0.48)
            ctx.lineTo(w * 0.25, h * 0.59)
            ctx.lineTo(w * 0.34, h * 0.48)
            ctx.lineTo(w * 0.34, h * 0.88)
            ctx.lineTo(w * 0.66, h * 0.88)
            ctx.lineTo(w * 0.66, h * 0.48)
            ctx.lineTo(w * 0.75, h * 0.59)
            ctx.lineTo(w * 0.92, h * 0.48)
            ctx.lineTo(w * 0.78, h * 0.25)
            ctx.lineTo(w * 0.65, h * 0.18)
            ctx.quadraticCurveTo(w * 0.5, h * 0.43, w * 0.35, h * 0.18)
            break

        case "calendar":
            ctx.roundRect(w * 0.13, h * 0.22, w * 0.74, h * 0.68, 3)
            ctx.moveTo(w * 0.13, h * 0.40)
            ctx.lineTo(w * 0.87, h * 0.40)
            ctx.moveTo(w * 0.32, h * 0.12)
            ctx.lineTo(w * 0.32, h * 0.30)
            ctx.moveTo(w * 0.68, h * 0.12)
            ctx.lineTo(w * 0.68, h * 0.30)
            ctx.moveTo(w * 0.32, h * 0.55)
            ctx.lineTo(w * 0.32, h * 0.56)
            ctx.moveTo(w * 0.50, h * 0.55)
            ctx.lineTo(w * 0.50, h * 0.56)
            ctx.moveTo(w * 0.68, h * 0.55)
            ctx.lineTo(w * 0.68, h * 0.56)
            break

        case "history":
            ctx.arc(w * 0.53, h * 0.53, w * 0.36, 0, Math.PI * 2)
            ctx.moveTo(w * 0.53, h * 0.31)
            ctx.lineTo(w * 0.53, h * 0.53)
            ctx.lineTo(w * 0.69, h * 0.62)
            ctx.moveTo(w * 0.12, h * 0.22)
            ctx.lineTo(w * 0.12, h * 0.43)
            ctx.lineTo(w * 0.32, h * 0.43)
            break

        case "settings":
            const cx = w / 2
            const cy = h / 2
            const outer = w * 0.39
            const inner = w * 0.30

            for (let i = 0; i < 16; i++) {
                const angle = i * Math.PI / 8
                const radius = i % 2 === 0 ? outer : inner
                const x = cx + Math.cos(angle) * radius
                const y = cy + Math.sin(angle) * radius

                if (i === 0)
                    ctx.moveTo(x, y)
                else
                    ctx.lineTo(x, y)
            }

            ctx.closePath()
            ctx.moveTo(cx + w * 0.13, cy)
            ctx.arc(cx, cy, w * 0.13, 0, Math.PI * 2)
            break
        }

        ctx.stroke()
    }

    onIconNameChanged: requestPaint()
    onIconColorChanged: requestPaint()
    onWidthChanged: requestPaint()
    onHeightChanged: requestPaint()
}
