// Copyright (c) 2020, Martin Sandsmark <martin.sandsmark@kde.org>
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of the <organization> nor the
//       names of its contributors may be used to endorse or promote products
//       derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "container_qt.h"

#include <string>

container_qt::container_qt(void)
{
    m_temp_surface  = new QImage(2, 2, QImage::Format_ARGB32);
    m_temp_cr       = new QPainter(m_temp_surface);
}

container_qt::~container_qt(void)
{
    clear_images();
    delete m_temp_cr;
    delete m_temp_surface;
}

litehtml::uint_ptr container_qt::create_font(const litehtml::tchar_t *faceName, int size, int weight, litehtml::font_style italic, unsigned int decoration, litehtml::font_metrics *fm)
{
    // TODO: easier way? Or do we need to use QFontDatabase::families() and match manually?
    litehtml::string_vector fonts;
    litehtml::split_string(faceName, fonts, ",");
    litehtml::trim(fonts[0]);
    QStringList families;
    for (litehtml::string_vector::iterator i = fonts.begin(); i != fonts.end(); i++) {
        families.append(QString::fromStdString(*i));
    }
    qDebug() << "Got families" << families;

    QFont *font = new QFont;
    font->setFamilies(families);

    int fc_weight = QFont::Normal;
    // TODO: I think we can just divide by 10, since setWeight just takes an int
    if (weight < 0) {
        qWarning() << "Invalid weight";
    } else if (weight < 150) {
        fc_weight = QFont::Thin;
    } else if (weight < 250) {
        fc_weight = QFont::ExtraLight;
    } else if (weight < 350) {
        fc_weight = QFont::Light;
    } else if (weight < 450) {
        fc_weight = QFont::Normal;
    } else if (weight < 550) {
        fc_weight = QFont::Medium;
    } else if (weight < 650) {
        fc_weight = QFont::DemiBold;
    } else if (weight < 750) {
        fc_weight = QFont::Bold;
    } else if (weight < 850) {
        fc_weight = QFont::ExtraBold;
    } else if (weight >= 950) {
        fc_weight = QFont::Black;
    }
    font->setWeight(fc_weight);

    font->setPixelSize(size); // TODO: not sure if litehtml uses point size of pixel size

    font->setItalic(italic == litehtml::fontStyleItalic);
    font->setStrikeOut(decoration & litehtml::font_decoration_linethrough);
    font->setUnderline(decoration & litehtml::font_decoration_underline);

    if (fm) {
        QFontMetrics metrics(*font);
        fm->ascent = metrics.ascent();
        fm->descent = metrics.descent();
        fm->height = metrics.height();
        fm->x_height = metrics.horizontalAdvance(QLatin1Char('x')); // TODO: cairo uses 'x', but usually people use 'M'...
    }


    return (litehtml::uint_ptr) font;
}

void container_qt::delete_font(litehtml::uint_ptr hFont)
{
    if (!hFont) {
        return;
    }
    delete reinterpret_cast<QFont*>(hFont);
}

int container_qt::text_width(const litehtml::tchar_t *text, litehtml::uint_ptr hFont)
{
    QFont *font = reinterpret_cast<QFont*>(hFont);
    QFontMetrics metrics(*font);
    return metrics.horizontalAdvance(QString::fromLocal8Bit(text));
}

void container_qt::draw_text(litehtml::uint_ptr hdc, const litehtml::tchar_t *text, litehtml::uint_ptr hFont, litehtml::web_color color, const litehtml::position &pos)
{
    QPainter *cr = reinterpret_cast<QPainter*>(hdc);

    cr->save();

    cr->setClipRegion(m_clip);
    set_color(cr, color);
    cr->setFont(*reinterpret_cast<QFont*>(hFont));

    // TODO no alignment?
    cr->drawText(pos.x, pos.y, QString::fromLocal8Bit(text));

    cr->restore();
}

int container_qt::get_default_font_size() const
{
    return QFont().pixelSize(); // TODO: don't know if litehtml uses points or pixels
}

void container_qt::draw_list_marker(litehtml::uint_ptr hdc, const litehtml::list_marker &marker)
{
    if (!marker.image.empty()) {
        /*litehtml::tstring url;
        make_url(marker.image.c_str(), marker.baseurl, url);

        lock_images_cache();
        images_map::iterator img_i = m_images.find(url.c_str());
        if(img_i != m_images.end())
        {
            if(img_i->second)
            {
                draw_txdib((QPainter*) hdc, img_i->second, marker.pos.x, marker.pos.y, marker.pos.width, marker.pos.height);
            }
        }
        unlock_images_cache();*/
    } else {
        switch (marker.marker_type) {
        case litehtml::list_style_type_circle: {
            draw_ellipse((QPainter *) hdc, marker.pos.x, marker.pos.y, marker.pos.width, marker.pos.height, marker.color, 0.5);
        }
        break;
        case litehtml::list_style_type_disc: {
            fill_ellipse((QPainter *) hdc, marker.pos.x, marker.pos.y, marker.pos.width, marker.pos.height, marker.color);
        }
        break;
        case litehtml::list_style_type_square:
            if (hdc) {
                QPainter *cr = (QPainter *) hdc;
                cairo_save(cr);

                cairo_new_path(cr);
                cairo_rectangle(cr, marker.pos.x, marker.pos.y, marker.pos.width, marker.pos.height);

                set_color(cr, marker.color);
                cairo_fill(cr);
                cairo_restore(cr);
            }
            break;
        default:
            /*do nothing*/
            break;
        }
    }
}

void container_qt::load_image(const litehtml::tchar_t *src, const litehtml::tchar_t *baseurl, bool redraw_on_ready)
{
    const QUrl url = qUrl(src, baseurl);

    // TODO: should we reload?
    if (m_images.contains(url) && !m_images[url].isNull()) {
        return;
    }
    m_images[url] = get_image(url, true);
}

void container_qt::get_image_size(const litehtml::tchar_t *src, const litehtml::tchar_t *baseurl, litehtml::size &sz)
{
    const QUrl url = qUrl(src, baseurl);

    if (m_images.contains(url)) {
        const QSize size = m_images[url].size();
        sz.width = size.width();
        sz.height = size.height();
    } else {
        sz.width    = 0;
        sz.height   = 0;
    }
}

void container_qt::draw_background(litehtml::uint_ptr hdc, const litehtml::background_paint &bg)
{
    QPainter *cr = reinterpret_cast<QPainter*>(hdc);
    cr->save();

    cr->setPen(QPen(Qt::transparent, 0));

    cr->setClipRect(qRect(bg.clip_box));

    if (bg.color.alpha) {
        cr->setBrush(qColor(bg.color));
        cr->drawRoundedRect(qRect(bg.border_box), bg.border_radius, bg.border_radius);
        cr->setClipRect(qRect(bg.border_box), Qt::IntersectClip);
    }

    const QUrl url = qUrl(bg.image.c_str(), bg.baseurl.c_str());
    if (m_images.contains(url) && !m_images[url].isNull()) {
        QImage image = m_images[url];

        switch (bg.repeat) {
        case litehtml::background_repeat_no_repeat:
            if (bg.image_size.width > 0 &&  bg.image_size.height > 0) {
                cr->drawImage(QRect(bg.position_x, bg.position_y,bg.image_size.width, bg.image_size.height), image);
            } else {
                cr->drawImage(bg.position_x, bg.position_y,bg.image_size.width, bg.image_size.height, image);
            }
            break;

        case litehtml::background_repeat_repeat_x:
            cr->fillRect(QRect(bg.clip_box.left(), bg.position_y, bg.clip_box.width, image.height()), QBrush(image));;
            break;

        case litehtml::background_repeat_repeat_y:
            cr->fillRect(QRect(bg.position_x, bg.clip_box.top(), image.width(), bg.clip_box.height), QBrush(image));;
            break;

        case litehtml::background_repeat_repeat:
            cr->fillRect(QRect(bg.clip_box.left(), bg.clip_box.top(), bg.clip_box.width, bg.clip_box.height), QBrush(image));;
            break;
        }
    }

    cr->restore();
}

void container_qt::make_url(const litehtml::tchar_t *url,    const litehtml::tchar_t *basepath, litehtml::tstring &out)
{
    out = url;
}

void container_qt::add_path_arc(QPainter *cr, double x, double y, double rx, double ry, double a1, double a2, bool neg)
{
    if (rx > 0 && ry > 0) {
        cr->drawArc(x, y, rx, ry, a1, a2);

//        cairo_save(cr);

//        QPainterranslate(cr, x, y);
//        cairo_scale(cr, 1, ry / rx);
//        QPainterranslate(cr, -x, -y);

//        if (neg) {
//            cairo_arc_negative(cr, x, y, rx, a1, a2);
//        } else {
//            cairo_arc(cr, x, y, rx, a1, a2);
//        }

//        cairo_restore(cr);
    } else {
//        cairo_move_to(cr, x, y);
    }
}

void container_qt::draw_borders(litehtml::uint_ptr hdc, const litehtml::borders &borders, const litehtml::position &draw_pos, bool root)
{
    // todo qpainterpath
    QPainter *cr = (QPainter *) hdc;
//    cairo_save(cr);
    apply_clip(cr);

//    cairo_new_path(cr);

    int bdr_top     = 0;
    int bdr_bottom  = 0;
    int bdr_left    = 0;
    int bdr_right   = 0;

    if (borders.top.width != 0 && borders.top.style > litehtml::border_style_hidden) {
        bdr_top = (int) borders.top.width;
    }
    if (borders.bottom.width != 0 && borders.bottom.style > litehtml::border_style_hidden) {
        bdr_bottom = (int) borders.bottom.width;
    }
    if (borders.left.width != 0 && borders.left.style > litehtml::border_style_hidden) {
        bdr_left = (int) borders.left.width;
    }
    if (borders.right.width != 0 && borders.right.style > litehtml::border_style_hidden) {
        bdr_right = (int) borders.right.width;
    }

    // draw right border
    if (bdr_right) {
        set_color(cr, borders.right.color);

        double r_top    = borders.radius.top_right_x;
        double r_bottom = borders.radius.bottom_right_x;

        if (r_top) {
            double end_angle    = 2 * M_PI;
            double start_angle  = end_angle - M_PI / 2.0  / ((double) bdr_top / (double) bdr_right + 1);

            add_path_arc(cr,
                         draw_pos.right() - r_top,
                         draw_pos.top() + r_top,
                         r_top - bdr_right,
                         r_top - bdr_right + (bdr_right - bdr_top),
                         end_angle,
                         start_angle, true);

            add_path_arc(cr,
                         draw_pos.right() - r_top,
                         draw_pos.top() + r_top,
                         r_top,
                         r_top,
                         start_angle,
                         end_angle, false);
        } else {
            cr->drawLine(draw_pos.right() - bdr_right, draw_pos.top() + bdr_top, draw_pos.right(), draw_pos.top());
//            cairo_move_to(cr, draw_pos.right() - bdr_right, draw_pos.top() + bdr_top);
//            cairo_line_to(cr, draw_pos.right(), draw_pos.top());
//            cairo_move_to(cr, draw_pos.right() - bdr_right, draw_pos.top() + bdr_top);
//            cairo_line_to(cr, draw_pos.right(), draw_pos.top());
        }

        if (r_bottom) {
//            cairo_line_to(cr, draw_pos.right(), draw_pos.bottom() - r_bottom);
            cr->drawLine(cr, draw_pos.right(), draw_pos.bottom() - r_bottom);

            double start_angle  = 0;
            double end_angle    = start_angle + M_PI / 2.0  / ((double) bdr_bottom / (double) bdr_right + 1);

            add_path_arc(cr,
                         draw_pos.right() - r_bottom,
                         draw_pos.bottom() - r_bottom,
                         r_bottom,
                         r_bottom,
                         start_angle,
                         end_angle, false);

            add_path_arc(cr,
                         draw_pos.right() - r_bottom,
                         draw_pos.bottom() - r_bottom,
                         r_bottom - bdr_right,
                         r_bottom - bdr_right + (bdr_right - bdr_bottom),
                         end_angle,
                         start_angle, true);
        } else {
            cairo_line_to(cr, draw_pos.right(), draw_pos.bottom());
            cairo_line_to(cr, draw_pos.right() - bdr_right, draw_pos.bottom() - bdr_bottom);
        }

        cairo_fill(cr);
    }

    // draw bottom border
    if (bdr_bottom) {
        set_color(cr, borders.bottom.color);

        double r_left   = borders.radius.bottom_left_x;
        double r_right  = borders.radius.bottom_right_x;

        if (r_left) {
            double start_angle  = M_PI / 2.0;
            double end_angle    = start_angle + M_PI / 2.0  / ((double) bdr_left / (double) bdr_bottom + 1);

            add_path_arc(cr,
                         draw_pos.left() + r_left,
                         draw_pos.bottom() - r_left,
                         r_left - bdr_bottom + (bdr_bottom - bdr_left),
                         r_left - bdr_bottom,
                         start_angle,
                         end_angle, false);

            add_path_arc(cr,
                         draw_pos.left() + r_left,
                         draw_pos.bottom() - r_left,
                         r_left,
                         r_left,
                         end_angle,
                         start_angle, true);
        } else {
            cairo_move_to(cr, draw_pos.left(), draw_pos.bottom());
            cairo_line_to(cr, draw_pos.left() + bdr_left, draw_pos.bottom() - bdr_bottom);
        }

        if (r_right) {
            cairo_line_to(cr, draw_pos.right() - r_right,   draw_pos.bottom());

            double end_angle    = M_PI / 2.0;
            double start_angle  = end_angle - M_PI / 2.0  / ((double) bdr_right / (double) bdr_bottom + 1);

            add_path_arc(cr,
                         draw_pos.right() - r_right,
                         draw_pos.bottom() - r_right,
                         r_right,
                         r_right,
                         end_angle,
                         start_angle, true);

            add_path_arc(cr,
                         draw_pos.right() - r_right,
                         draw_pos.bottom() - r_right,
                         r_right - bdr_bottom + (bdr_bottom - bdr_right),
                         r_right - bdr_bottom,
                         start_angle,
                         end_angle, false);
        } else {
            cairo_line_to(cr, draw_pos.right() - bdr_right, draw_pos.bottom() - bdr_bottom);
            cairo_line_to(cr, draw_pos.right(), draw_pos.bottom());
        }

        cairo_fill(cr);
    }

    // draw top border
    if (bdr_top) {
        set_color(cr, borders.top.color);

        double r_left   = borders.radius.top_left_x;
        double r_right  = borders.radius.top_right_x;

        if (r_left) {
            double end_angle    = M_PI * 3.0 / 2.0;
            double start_angle  = end_angle - M_PI / 2.0  / ((double) bdr_left / (double) bdr_top + 1);

            add_path_arc(cr,
                         draw_pos.left() + r_left,
                         draw_pos.top() + r_left,
                         r_left,
                         r_left,
                         end_angle,
                         start_angle, true);

            add_path_arc(cr,
                         draw_pos.left() + r_left,
                         draw_pos.top() + r_left,
                         r_left - bdr_top + (bdr_top - bdr_left),
                         r_left - bdr_top,
                         start_angle,
                         end_angle, false);
        } else {
            cairo_move_to(cr, draw_pos.left(), draw_pos.top());
            cairo_line_to(cr, draw_pos.left() + bdr_left, draw_pos.top() + bdr_top);
        }

        if (r_right) {
            cairo_line_to(cr, draw_pos.right() - r_right,   draw_pos.top() + bdr_top);

            double start_angle  = M_PI * 3.0 / 2.0;
            double end_angle    = start_angle + M_PI / 2.0  / ((double) bdr_right / (double) bdr_top + 1);

            add_path_arc(cr,
                         draw_pos.right() - r_right,
                         draw_pos.top() + r_right,
                         r_right - bdr_top + (bdr_top - bdr_right),
                         r_right - bdr_top,
                         start_angle,
                         end_angle, false);

            add_path_arc(cr,
                         draw_pos.right() - r_right,
                         draw_pos.top() + r_right,
                         r_right,
                         r_right,
                         end_angle,
                         start_angle, true);
        } else {
            cairo_line_to(cr, draw_pos.right() - bdr_right, draw_pos.top() + bdr_top);
            cairo_line_to(cr, draw_pos.right(), draw_pos.top());
        }

        cairo_fill(cr);
    }

    // draw left border
    if (bdr_left) {
        set_color(cr, borders.left.color);

        double r_top    = borders.radius.top_left_x;
        double r_bottom = borders.radius.bottom_left_x;

        if (r_top) {
            double start_angle  = M_PI;
            double end_angle    = start_angle + M_PI / 2.0  / ((double) bdr_top / (double) bdr_left + 1);

            add_path_arc(cr,
                         draw_pos.left() + r_top,
                         draw_pos.top() + r_top,
                         r_top - bdr_left,
                         r_top - bdr_left + (bdr_left - bdr_top),
                         start_angle,
                         end_angle, false);

            add_path_arc(cr,
                         draw_pos.left() + r_top,
                         draw_pos.top() + r_top,
                         r_top,
                         r_top,
                         end_angle,
                         start_angle, true);
        } else {
            cairo_move_to(cr, draw_pos.left() + bdr_left, draw_pos.top() + bdr_top);
            cairo_line_to(cr, draw_pos.left(), draw_pos.top());
        }

        if (r_bottom) {
            cairo_line_to(cr, draw_pos.left(),  draw_pos.bottom() - r_bottom);

            double end_angle    = M_PI;
            double start_angle  = end_angle - M_PI / 2.0  / ((double) bdr_bottom / (double) bdr_left + 1);

            add_path_arc(cr,
                         draw_pos.left() + r_bottom,
                         draw_pos.bottom() - r_bottom,
                         r_bottom,
                         r_bottom,
                         end_angle,
                         start_angle, true);

            add_path_arc(cr,
                         draw_pos.left() + r_bottom,
                         draw_pos.bottom() - r_bottom,
                         r_bottom - bdr_left,
                         r_bottom - bdr_left + (bdr_left - bdr_bottom),
                         start_angle,
                         end_angle, false);
        } else {
            cairo_line_to(cr, draw_pos.left(),  draw_pos.bottom());
            cairo_line_to(cr, draw_pos.left() + bdr_left,   draw_pos.bottom() - bdr_bottom);
        }

        cairo_fill(cr);
    }
    cairo_restore(cr);
}

void container_qt::transform_text(litehtml::tstring &text, litehtml::text_transform tt)
{

}

void container_qt::set_clip(const litehtml::position &pos, const litehtml::border_radiuses &bdr_radius, bool valid_x, bool valid_y)
{
    litehtml::position clip_pos = pos;
    litehtml::position client_pos;
    get_client_rect(client_pos);
    if (!valid_x) {
        clip_pos.x      = client_pos.x;
        clip_pos.width  = client_pos.width;
    }
    if (!valid_y) {
        clip_pos.y      = client_pos.y;
        clip_pos.height = client_pos.height;
    }
    m_clips.emplace_back(clip_pos, bdr_radius);
}

void container_qt::del_clip()
{
    if (!m_clips.empty()) {
        m_clips.pop_back();
    }
}

void container_qt::apply_clip(QPainter *cr)
{
    for (const auto &clip_box : m_clips) {
        rounded_rectangle(cr, clip_box.box, clip_box.radius);
        cairo_clip(cr);
    }
}

void container_qt::draw_ellipse(QPainter *cr, int x, int y, int width, int height, const litehtml::web_color &color, int line_width)
{
    if (!cr) {
        return;
    }
    cairo_save(cr);

    apply_clip(cr);

    cairo_new_path(cr);

    QPainterranslate(cr, x + width / 2.0, y + height / 2.0);
    cairo_scale(cr, width / 2.0, height / 2.0);
    cairo_arc(cr, 0, 0, 1, 0, 2 * M_PI);

    set_color(cr, color);
    cairo_set_line_width(cr, line_width);
    cairo_stroke(cr);

    cairo_restore(cr);
}

void container_qt::fill_ellipse(QPainter *cr, int x, int y, int width, int height, const litehtml::web_color &color)
{
    if (!cr) {
        return;
    }
    cairo_save(cr);

    apply_clip(cr);

    cairo_new_path(cr);

    QPainterranslate(cr, x + width / 2.0, y + height / 2.0);
    cairo_scale(cr, width / 2.0, height / 2.0);
    cairo_arc(cr, 0, 0, 1, 0, 2 * M_PI);

    set_color(cr, color);
    cairo_fill(cr);

    cairo_restore(cr);
}

void container_qt::clear_images()
{
    /*  for(images_map::iterator i = m_images.begin(); i != m_images.end(); i++)
        {
            if(i->second)
            {
                delete i->second;
            }
        }
        m_images.clear();
    */
}

const litehtml::tchar_t *container_qt::get_default_font_name() const
{
    return "Times New Roman";
}

std::shared_ptr<litehtml::element>  container_qt::create_element(const litehtml::tchar_t *tag_name,
        const litehtml::string_map &attributes,
        const std::shared_ptr<litehtml::document> &doc)
{
    return 0;
}

void container_qt::rounded_rectangle(QPainter *cr, const litehtml::position &pos, const litehtml::border_radiuses &radius)
{
    cairo_new_path(cr);
    if (radius.top_left_x) {
        cairo_arc(cr, pos.left() + radius.top_left_x, pos.top() + radius.top_left_x, radius.top_left_x, M_PI, M_PI * 3.0 / 2.0);
    } else {
        cairo_move_to(cr, pos.left(), pos.top());
    }

    cairo_line_to(cr, pos.right() - radius.top_right_x, pos.top());

    if (radius.top_right_x) {
        cairo_arc(cr, pos.right() - radius.top_right_x, pos.top() + radius.top_right_x, radius.top_right_x, M_PI * 3.0 / 2.0, 2.0 * M_PI);
    }

    cairo_line_to(cr, pos.right(), pos.bottom() - radius.bottom_right_x);

    if (radius.bottom_right_x) {
        cairo_arc(cr, pos.right() - radius.bottom_right_x, pos.bottom() - radius.bottom_right_x, radius.bottom_right_x, 0, M_PI / 2.0);
    }

    cairo_line_to(cr, pos.left() - radius.bottom_left_x, pos.bottom());

    if (radius.bottom_left_x) {
        cairo_arc(cr, pos.left() + radius.bottom_left_x, pos.bottom() - radius.bottom_left_x, radius.bottom_left_x, M_PI / 2.0, M_PI);
    }
}

void container_qt::draw_pixbuf(QPainter *cr, const Glib::RefPtr<Gdk::Pixbuf> &bmp, int x, int y, int cx, int cy)
{
    cairo_save(cr);

    {
        Cairo::RefPtr<Cairo::Context> crobj(new Cairo::Context(cr, false));

        cairo_matrix_t flib_m;
        cairo_matrix_init(&flib_m, 1, 0, 0, -1, 0, 0);

        if (cx != bmp->get_width() || cy != bmp->get_height()) {
            Glib::RefPtr<Gdk::Pixbuf> new_img = bmp->scale_simple(cx, cy, Gdk::INTERP_BILINEAR);;
            Gdk::Cairo::set_source_pixbuf(crobj, new_img, x, y);
            crobj->paint();
        } else {
            Gdk::Cairo::set_source_pixbuf(crobj, bmp, x, y);
            crobj->paint();
        }
    }

    cairo_restore(cr);
}

QImage *container_qt::surface_from_pixbuf(const Glib::RefPtr<Gdk::Pixbuf> &bmp)
{
    QImage *ret = NULL;

    if (bmp->get_has_alpha()) {
        ret = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, bmp->get_width(), bmp->get_height());
    } else {
        ret = cairo_image_surface_create(CAIRO_FORMAT_RGB24, bmp->get_width(), bmp->get_height());
    }

    Cairo::RefPtr<Cairo::Surface> surface(new Cairo::Surface(ret, false));
    Cairo::RefPtr<Cairo::Context> ctx = Cairo::Context::create(surface);
    Gdk::Cairo::set_source_pixbuf(ctx, bmp, 0.0, 0.0);
    ctx->paint();

    return ret;
}

void container_qt::get_media_features(litehtml::media_features &media) const
{
    litehtml::position client;
    get_client_rect(client);
    media.type          = litehtml::media_type_screen;
    media.width         = client.width;
    media.height        = client.height;
    media.device_width  = Gdk::screen_width();
    media.device_height = Gdk::screen_height();
    media.color         = 8;
    media.monochrome    = 0;
    media.color_index   = 256;
    media.resolution    = 96;
}

void container_qt::get_language(litehtml::tstring &language, litehtml::tstring &culture) const
{
    language = _t("en");
    culture = _t("");
}

void container_qt::link(const std::shared_ptr<litehtml::document> &ptr, const litehtml::element::ptr &el)
{

}
