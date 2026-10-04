// Консольные тесты модели: запись/чтение XML, обработка ошибок, неизменность объекта при ошибке.
// Запуск: ./build/test_model ; код возврата 0 = все проверки прошли.

#include "Dimensions.h"
#include "StockItem.h"
#include "SparePartType.h"
#include "PartGroup.h"
#include "PartCatalog.h"
#include "Warehouse.h"
#include "Product.h"
#include "SupplySystem.h"
#include "NamedEntity.h"
#include "XmlUtil.h"

#include <QByteArray>
#include <QDebug>
#include <QFile>
#include <QList>
#include <QTemporaryDir>
#include <QtGlobal>

// ---------------------------------------------------------------- инфраструктура

static int failures = 0;

#define CHECK(cond)                                                  \
    do {                                                             \
        if (cond) qInfo("  OK    %s", #cond);                        \
        else { qWarning("  FAIL  %s  (строка %d)", #cond, __LINE__); \
               ++failures; }                                         \
    } while (0)

static void section(const char *title) { qInfo("== %s ==", title); }

// Сравнение дробных чисел с допуском: в XML мы пишем 2-3 знака после запятой.
static bool near(double a, double b) { return qAbs(a - b) < 0.0005; }

static QString ru(const char *s) { return QString::fromUtf8(s); }

// Встаёт на первый открывающий тег строки XML и вызывает loadXml у объекта.
// Возвращает true, если ошибок разбора нет. Текст ошибки (если нужен) кладёт в err.
template <class T>
static bool tryLoad(const char *xml, T &obj, QString *err = nullptr)
{
    QXmlStreamReader r{QByteArray(xml)};
    r.readNextStartElement();
    obj.loadXml(r);
    if (err) *err = r.errorString();
    return !r.hasError();
}

// Запись объекта в XML-строку.
template <class T>
static QByteArray toXml(const T &obj)
{
    QByteArray data;
    QXmlStreamWriter w(&data);
    obj.saveXml(w);
    return data;
}

// Минимальный потомок NamedEntity, чтобы проверить protected-методы loadId/loadName.
class TestNamed : public NamedEntity {
public:
    using NamedEntity::NamedEntity;
    void saveXml(QXmlStreamWriter &w) const override {
        w.writeStartElement("item");
        saveId(w);
        saveName(w);
        w.writeEndElement();
    }
    void loadXml(QXmlStreamReader &r) override {
        TestNamed tmp;
        if (!tmp.loadId(r) || !tmp.loadName(r))
            return;
        *this = tmp;
        r.skipCurrentElement();
    }
};

// ---------------------------------------------------------------- XmlUtil

static void testXmlUtil()
{
    section("XmlUtil: readString читает именно запрошенный атрибут");
    {
        QXmlStreamReader r{QByteArray(R"(<a title="Двигатель"/>)")};
        r.readNextStartElement();
        QString s;
        CHECK(XmlUtil::readString(r, "title", s));
        CHECK(s == ru("Двигатель"));
    }

    section("XmlUtil: readString убирает пробелы по краям, нет атрибута = ошибка");
    {
        QXmlStreamReader r{QByteArray(R"(<a title="  X  "/>)")};
        r.readNextStartElement();
        QString s;
        CHECK(XmlUtil::readString(r, "title", s) && s == "X");

        QXmlStreamReader r2{QByteArray(R"(<a/>)")};
        r2.readNextStartElement();
        QString s2 = "keep";
        CHECK(!XmlUtil::readString(r2, "title", s2));
        CHECK(r2.hasError() && s2 == "keep");
    }

    section("XmlUtil: readInt принимает 0 и отрицательные (диапазон проверяет вызывающий)");
    {
        QXmlStreamReader r{QByteArray(R"(<a q="0" n="-3" bad="x1"/>)")};
        r.readNextStartElement();
        int v = 99;
        CHECK(XmlUtil::readInt(r, "q", v) && v == 0);
        CHECK(XmlUtil::readInt(r, "n", v) && v == -3);
        v = 99;
        CHECK(!XmlUtil::readInt(r, "bad", v));
        CHECK(v == 99);                          // out не изменился
    }

    section("XmlUtil: readDouble отвергает мусор и nan/inf");
    {
        for (const char *bad : {R"(<a v="abc"/>)", R"(<a v="nan"/>)", R"(<a v="inf"/>)"}) {
            QXmlStreamReader r{QByteArray(bad)};
            r.readNextStartElement();
            double v = 5;
            CHECK(!XmlUtil::readDouble(r, "v", v));
            CHECK(near(v, 5));
        }
        QXmlStreamReader ok{QByteArray(R"(<a v="-2.5"/>)")};
        ok.readNextStartElement();
        double v = 0;
        CHECK(XmlUtil::readDouble(ok, "v", v) && near(v, -2.5));
    }

    section("XmlUtil: readDate");
    {
        QXmlStreamReader r{QByteArray(R"(<a d="2018-01-12" bad="2018-13-45"/>)")};
        r.readNextStartElement();
        QDate d;
        CHECK(XmlUtil::readDate(r, "d", d) && d == QDate(2018, 1, 12));
        QDate keep(2020, 5, 5);
        CHECK(!XmlUtil::readDate(r, "bad", keep));
        CHECK(keep == QDate(2020, 5, 5));
    }
}

// ---------------------------------------------------------------- Entity / NamedEntity

static void testNamedEntity()
{
    section("NamedEntity: запись и чтение (русское имя)");
    {
        TestNamed src(5, ru("Двигатель"));
        const QByteArray xml = toXml(src);
        qInfo().noquote() << "XML:" << QString::fromUtf8(xml);

        QXmlStreamReader r(xml);
        r.readNextStartElement();
        TestNamed dst;
        dst.loadXml(r);
        CHECK(!r.hasError());
        CHECK(dst.id() == 5);
        CHECK(dst.name() == ru("Двигатель"));
    }

    section("NamedEntity: id должен быть положительным");
    {
        for (const char *bad : {R"(<item id="0" name="X"/>)",
                                R"(<item id="-4" name="X"/>)",
                                R"(<item id="abc" name="X"/>)",
                                R"(<item name="X"/>)"}) {
            TestNamed t(7, "OLD");
            QString err;
            CHECK(!tryLoad(bad, t, &err));
            qInfo().noquote() << "Ошибка:" << err;
            CHECK(t.id() == 7 && t.name() == "OLD");
        }
    }

    section("NamedEntity: имя не должно быть пустым");
    {
        for (const char *bad : {R"(<item id="1" name=""/>)",
                                R"(<item id="1" name="   "/>)",
                                R"(<item id="1"/>)"}) {
            TestNamed t(7, "OLD");
            QString err;
            CHECK(!tryLoad(bad, t, &err));
            qInfo().noquote() << "Ошибка:" << err;
            CHECK(t.id() == 7 && t.name() == "OLD");
        }
    }

    section("NamedEntity: пробелы по краям имени обрезаются");
    {
        TestNamed t;
        CHECK(tryLoad(R"(<item id="1" name="  Трансмиссия "/>)", t));
        CHECK(t.name() == ru("Трансмиссия"));
    }
}

// ---------------------------------------------------------------- Dimensions

static void testDimensions()
{
    section("Dimensions: запись и чтение обратно");
    {
        const QByteArray xml = toXml(Dimensions(0.4, 0.4, 0.5));
        qInfo().noquote() << "XML:" << QString::fromUtf8(xml);

        QXmlStreamReader r(xml);
        r.readNextStartElement();
        Dimensions dst;
        dst.loadXml(r);
        CHECK(!r.hasError());
        CHECK(near(dst.length(), 0.4));
        CHECK(near(dst.width(),  0.4));
        CHECK(near(dst.height(), 0.5));
        CHECK(dst.isValid());
        CHECK(r.isEndElement());         // контракт: остановились на закрывающем теге
    }

    section("Dimensions: плохой ввод, объект не меняется");
    {
        struct Case { const char *name; const char *xml; };
        const Case cases[] = {
            {"отрицательный габарит", R"(<dimensions length="-1" width="0.3" height="0.4"/>)"},
            {"нулевой габарит",       R"(<dimensions length="0" width="0.3" height="0.4"/>)"},
            {"нет атрибута width",    R"(<dimensions length="0.3" height="0.4"/>)"},
            {"не число",              R"(<dimensions length="abc" width="0.3" height="0.4"/>)"},
        };
        for (const Case &c : cases) {
            Dimensions d(1, 2, 3);
            QString err;
            qInfo("  - %s", c.name);
            CHECK(!tryLoad(c.xml, d, &err));
            qInfo().noquote() << "    Ошибка:" << err;
            CHECK(near(d.length(), 1) && near(d.width(), 2) && near(d.height(), 3));
        }
    }
}

// ---------------------------------------------------------------- StockItem

static void testStockItem()
{
    section("StockItem: запись и чтение обратно");
    {
        const QByteArray xml = toXml(StockItem(3, 12, 10200.0, QDate(2018, 1, 12)));
        qInfo().noquote() << "XML:" << QString::fromUtf8(xml);

        QXmlStreamReader r(xml);
        r.readNextStartElement();
        StockItem dst;
        dst.loadXml(r);
        CHECK(!r.hasError());
        CHECK(dst.partId() == 3);
        CHECK(dst.quantity() == 12);
        CHECK(near(dst.price(), 10200.0));
        CHECK(dst.deliveryDate() == QDate(2018, 1, 12));
        CHECK(r.isEndElement());
    }

    section("StockItem: плохой ввод, объект не меняется");
    {
        struct Case { const char *name; const char *xml; };
        const Case cases[] = {
            {"плохая дата",
             R"(<stock partId="3" quantity="2" price="10.00" deliveryDate="2018-13-45"/>)"},
            {"отрицательное количество",
             R"(<stock partId="3" quantity="-2" price="10.00" deliveryDate="2018-01-12"/>)"},
            {"отрицательная цена",
             R"(<stock partId="3" quantity="2" price="-5" deliveryDate="2018-01-12"/>)"},
            {"нет partId",
             R"(<stock quantity="2" price="10.00" deliveryDate="2018-01-12"/>)"},
            {"partId = 0",
             R"(<stock partId="0" quantity="2" price="10.00" deliveryDate="2018-01-12"/>)"},
        };
        for (const Case &c : cases) {
            StockItem s(1, 1, 1.0, QDate(2020, 5, 5));
            QString err;
            qInfo("  - %s", c.name);
            CHECK(!tryLoad(c.xml, s, &err));
            qInfo().noquote() << "    Ошибка:" << err;
            CHECK(s.partId() == 1 && s.quantity() == 1 && near(s.price(), 1.0)
                  && s.deliveryDate() == QDate(2020, 5, 5));
        }
    }

    section("StockItem: граничные допустимые значения (quantity=0, price=0)");
    {
        StockItem s;
        CHECK(tryLoad(R"(<stock partId="3" quantity="0" price="0" deliveryDate="2018-01-12"/>)", s));
        CHECK(s.quantity() == 0 && near(s.price(), 0.0));
    }
}

// ---------------------------------------------------------------- SparePartType

static void testSparePartType()
{
    section("SparePartType: запись и чтение обратно");
    {
        SparePartType src(3, ru("РПМ в сборе 2121"), 11.5, 10200.0, Dimensions(0.4, 0.4, 0.3));
        const QByteArray xml = toXml(src);
        qInfo().noquote() << "XML:" << QString::fromUtf8(xml);

        QXmlStreamReader r(xml);
        r.readNextStartElement();
        SparePartType dst;
        dst.loadXml(r);
        CHECK(!r.hasError());
        CHECK(dst.id() == 3);
        CHECK(dst.name() == ru("РПМ в сборе 2121"));
        CHECK(near(dst.mass(), 11.5));
        CHECK(near(dst.price(), 10200.0));
        CHECK(near(dst.dimensions().height(), 0.3));
        CHECK(r.isEndElement());
    }

    section("SparePartType: контракт loadXml, соседний элемент читается следом");
    {
        QXmlStreamReader r{QByteArray(
            R"(<group id="2" name="Трансмиссия">)"
            R"(<part id="1" name="Первая" mass="1" price="1"><dimensions length="1" width="1" height="1"/></part>)"
            R"(<part id="2" name="Вторая" mass="2" price="2"><dimensions length="1" width="1" height="1"/></part>)"
            R"(</group>)")};
        r.readNextStartElement();                       // <group>
        QList<SparePartType> parts;
        while (r.readNextStartElement()) {              // <part> ... <part>
            SparePartType p;
            p.loadXml(r);
            if (r.hasError()) break;
            parts.append(p);
        }
        CHECK(!r.hasError());
        CHECK(parts.size() == 2);
        CHECK(parts.size() == 2 && parts[1].name() == ru("Вторая"));
    }

    section("SparePartType: плохой ввод, объект не меняется");
    {
        struct Case { const char *name; const char *xml; };
        const Case cases[] = {
            {"нет dimensions",
             R"(<part id="3" name="X" mass="2" price="5"></part>)"},
            {"невалидные габариты внутри",
             R"(<part id="3" name="X" mass="2" price="5"><dimensions length="1" width="-1" height="1"/></part>)"},
            {"два dimensions",
             R"(<part id="3" name="X" mass="2" price="5"><dimensions length="1" width="1" height="1"/><dimensions length="2" width="2" height="2"/></part>)"},
            {"обрезанный XML (нет </part>)",
             R"(<part id="3" name="NEW" mass="2" price="5"><dimensions length="1" width="1" height="1"/>)"},
            {"id = 0",
             R"(<part id="0" name="X" mass="2" price="5"><dimensions length="1" width="1" height="1"/></part>)"},
            {"пустое имя",
             R"(<part id="3" name="" mass="2" price="5"><dimensions length="1" width="1" height="1"/></part>)"},
            {"масса = 0",
             R"(<part id="3" name="X" mass="0" price="5"><dimensions length="1" width="1" height="1"/></part>)"},
            {"отрицательная цена",
             R"(<part id="3" name="X" mass="2" price="-1"><dimensions length="1" width="1" height="1"/></part>)"},
        };
        for (const Case &c : cases) {
            SparePartType p(7, "OLD", 1, 1, Dimensions(1, 1, 1));
            QString err;
            qInfo("  - %s", c.name);
            CHECK(!tryLoad(c.xml, p, &err));
            qInfo().noquote() << "    Ошибка:" << err;
            CHECK(p.id() == 7 && p.name() == "OLD" && near(p.mass(), 1));
        }
    }

    section("SparePartType: допустимые особые случаи");
    {
        SparePartType p;
        CHECK(tryLoad(R"(<part id="3" name="X" mass="2" price="5"><note>привет</note><dimensions length="1" width="1" height="1"/></part>)", p));
        CHECK(p.id() == 3);                              // неизвестный элемент пропущен

        SparePartType z;
        CHECK(tryLoad(R"(<part id="4" name="X" mass="2" price="0"><dimensions length="1" width="1" height="1"/></part>)", z));
        CHECK(near(z.price(), 0.0));                     // нулевая цена допустима
    }
}

// ---------------------------------------------------------------- PartGroup

static SparePartType makePart(int id, const char *name)
{
    return SparePartType(id, ru(name), 1, 1, Dimensions(1, 1, 1));
}

// XML типа ЗЧ для вставки в строки тестов
#define PART_XML(id, name) \
    R"(<part id=")" #id R"(" name=")" name R"(" mass="1" price="1"><dimensions length="1" width="1" height="1"/></part>)"

static void testPartGroup()
{
    section("PartGroup: addPart / findPart / removePart");
    {
        PartGroup g(1, ru("Трансмиссия"));
        CHECK(g.parts().isEmpty());
        CHECK(g.addPart(makePart(5, "e")));
        CHECK(g.addPart(makePart(1, "a")));
        CHECK(g.parts().size() == 2);
        CHECK(g.parts()[0].id() == 5 && g.parts()[1].id() == 1);   // порядок добавления
        CHECK(!g.addPart(makePart(5, "dup")));                     // дубликат id
        CHECK(g.parts().size() == 2);
        CHECK(!g.addPart(SparePartType()));                        // невалидный тип
        CHECK(g.parts().size() == 2);

        CHECK(g.findPart(1) != nullptr && g.findPart(1)->name() == "a");
        CHECK(g.findPart(99) == nullptr);

        g.findPart(5)->setMass(42.0);                              // изменение через указатель
        CHECK(near(g.parts()[0].mass(), 42.0));

        CHECK(g.removePart(5));
        CHECK(!g.removePart(5));
        CHECK(g.parts().size() == 1 && g.parts()[0].id() == 1);
    }

    section("PartGroup: isValid");
    {
        PartGroup empty(1, ru("Двигатель"));
        CHECK(empty.isValid());                                    // пустая группа допустима
        CHECK(!PartGroup(0, "X").isValid());                       // id = 0
        CHECK(!PartGroup(1, "").isValid());                        // пустое имя

        PartGroup g(2, "G");
        g.addPart(makePart(1, "a"));
        CHECK(g.isValid());
        g.findPart(1)->setMass(0);                                 // тип стал невалидным
        CHECK(!g.isValid());
    }

    section("PartGroup: копия независима от оригинала");
    {
        PartGroup g(1, "G");
        g.addPart(makePart(1, "a"));
        PartGroup copy = g;
        copy.findPart(1)->setMass(99);
        copy.addPart(makePart(2, "b"));
        CHECK(near(g.findPart(1)->mass(), 1.0));
        CHECK(g.parts().size() == 1 && copy.parts().size() == 2);
    }

    section("PartGroup: запись и чтение, порядок сохраняется");
    {
        PartGroup src(2, ru("Трансмиссия"));
        src.addPart(makePart(5, "Пятая"));
        src.addPart(makePart(1, "Первая"));
        const QByteArray xml = toXml(src);
        qInfo().noquote() << "XML:" << QString::fromUtf8(xml);

        QXmlStreamReader r(xml);
        r.readNextStartElement();
        PartGroup dst;
        dst.loadXml(r);
        CHECK(!r.hasError());
        CHECK(dst.id() == 2 && dst.name() == ru("Трансмиссия"));
        CHECK(dst.parts().size() == 2);
        CHECK(dst.parts().size() == 2 && dst.parts()[0].id() == 5 && dst.parts()[1].id() == 1);
        CHECK(dst.parts().size() == 2 && dst.parts()[0].name() == ru("Пятая"));
        CHECK(r.isEndElement());
    }

    section("PartGroup: пустая группа (оба вида записи)");
    {
        PartGroup g1, g2;
        CHECK(tryLoad(R"(<group id="1" name="Двигатель"/>)", g1));
        CHECK(g1.parts().isEmpty() && g1.isValid());
        CHECK(tryLoad(R"(<group id="1" name="Двигатель"></group>)", g2));
        CHECK(g2.parts().isEmpty());
        qInfo().noquote() << "XML пустой группы:" << QString::fromUtf8(toXml(g1));
    }

    section("PartGroup: плохой ввод, объект не меняется");
    {
        struct Case { const char *name; QByteArray xml; };
        const Case cases[] = {
            {"дубликат id типов",
             QByteArray(R"(<group id="2" name="T">)") + PART_XML(1, "a") + PART_XML(1, "b") + "</group>"},
            {"невалидный тип внутри",
             QByteArray(R"(<group id="2" name="T"><part id="1" name="X" mass="0" price="1"><dimensions length="1" width="1" height="1"/></part></group>)")},
            {"обрезанный XML",
             QByteArray(R"(<group id="2" name="T">)") + PART_XML(1, "a")},
            {"id группы = 0",
             QByteArray(R"(<group id="0" name="T">)") + PART_XML(1, "a") + "</group>"},
            {"пустое имя группы",
             QByteArray(R"(<group id="2" name="">)") + PART_XML(1, "a") + "</group>"},
        };
        for (const Case &c : cases) {
            PartGroup g(9, "OLD");
            g.addPart(makePart(7, "keep"));
            QString err;
            qInfo("  - %s", c.name);
            CHECK(!tryLoad(c.xml.constData(), g, &err));
            qInfo().noquote() << "    Ошибка:" << err;
            CHECK(g.id() == 9 && g.name() == "OLD" && g.parts().size() == 1 && g.parts()[0].id() == 7);
        }
    }

    section("PartGroup: неизвестный элемент пропускается");
    {
        PartGroup g;
        const QByteArray xml = QByteArray(R"(<group id="2" name="T"><note>x</note>)") + PART_XML(1, "a") + "</group>";
        CHECK(tryLoad(xml.constData(), g));
        CHECK(g.parts().size() == 1);
    }

    section("PartGroup: контракт, следующая группа читается после предыдущей");
    {
        const QByteArray xml = QByteArray(R"(<catalog>)")
            + R"(<group id="1" name="Двигатель"/>)"
            + R"(<group id="2" name="Трансмиссия">)" + PART_XML(1, "a") + "</group>"
            + "</catalog>";
        QXmlStreamReader r(xml);
        r.readNextStartElement();                       // <catalog>
        QList<PartGroup> groups;
        while (r.readNextStartElement()) {
            PartGroup g;
            g.loadXml(r);
            if (r.hasError()) break;
            groups.append(g);
        }
        CHECK(!r.hasError());
        CHECK(groups.size() == 2);
        CHECK(groups.size() == 2 && groups[1].name() == ru("Трансмиссия") && groups[1].parts().size() == 1);
    }
}

// ---------------------------------------------------------------- PartCatalog

// Группа с типами, чьи id перечислены (имена "p<id>").
static PartGroup makeGroup(int id, const char *name, std::initializer_list<int> partIds = {})
{
    PartGroup g(id, ru(name));
    for (int pid : partIds)
        g.addPart(makePart(pid, qPrintable(QString("p%1").arg(pid))));
    return g;
}

static void testPartCatalog()
{
    section("PartCatalog: nextGroupId / nextPartId считаются по ВСЕМ группам");
    {
        PartCatalog c;
        CHECK(c.nextGroupId() == 1 && c.nextPartId() == 1);        // пустой каталог
        c.addGroup(makeGroup(1, "A", {4, 5}));
        c.addGroup(makeGroup(2, "B", {1, 2, 3}));
        CHECK(c.nextGroupId() == 3);
        CHECK(c.nextPartId() == 6);                                // max = 5 из ПЕРВОЙ группы
    }

    section("PartCatalog: addGroup");
    {
        PartCatalog c;
        CHECK(c.addGroup(makeGroup(1, "A", {1, 2})));
        CHECK(c.addGroup(makeGroup(2, "Пустая")));                 // пустая группа допустима
        CHECK(!c.addGroup(makeGroup(1, "Дубль id группы")));
        CHECK(!c.addGroup(makeGroup(0, "id=0")));                  // невалидная
        CHECK(!c.addGroup(makeGroup(3, "Тип из другой группы", {2, 9})));   // id 2 уже есть в группе A
        CHECK(c.groups().size() == 2);
        CHECK(c.findPart(9) == nullptr);                           // группа не добавилась частично
    }

    section("PartCatalog: addPart");
    {
        PartCatalog c;
        c.addGroup(makeGroup(10, "A", {1}));
        c.addGroup(makeGroup(20, "B", {2}));
        CHECK(c.addPart(20, makePart(3, "n")));
        CHECK(!c.addPart(99, makePart(4, "n")));                   // нет такой группы
        CHECK(!c.addPart(10, makePart(2, "dup")));                 // id есть в ДРУГОЙ группе
        CHECK(!c.addPart(10, makePart(1, "dup")));                 // id есть в этой же группе
        CHECK(!c.addPart(10, SparePartType()));                    // невалидный тип
        CHECK(c.findGroup(10)->parts().size() == 1);
        CHECK(c.findGroup(20)->parts().size() == 2);
    }

    section("PartCatalog: removeGroup / removePart");
    {
        PartCatalog c;
        c.addGroup(makeGroup(1, "Пустая"));
        c.addGroup(makeGroup(2, "С типом", {7}));
        CHECK(!c.removeGroup(2));                                  // в группе есть типы
        CHECK(!c.removeGroup(99));                                 // нет такой
        CHECK(c.removeGroup(1));                                   // пустая удаляется
        CHECK(c.groups().size() == 1);
        CHECK(!c.removePart(99));
        CHECK(c.removePart(7));
        CHECK(c.removeGroup(2));                                   // теперь пустая
        CHECK(c.groups().isEmpty());
    }

    section("PartCatalog: поиск (id групп 10 и 20, чтобы id != индекс)");
    {
        PartCatalog c;
        c.addGroup(makeGroup(10, "A", {1}));
        c.addGroup(makeGroup(20, "B", {2}));
        CHECK(c.findGroup(20) != nullptr && c.findGroup(20)->name() == "B");
        CHECK(c.findGroup(99) == nullptr);
        CHECK(c.groupOfPart(2) != nullptr && c.groupOfPart(2)->id() == 20);
        CHECK(c.groupOfPart(99) == nullptr);
        CHECK(c.findPart(2) != nullptr && c.findPart(2)->name() == "p2");
        CHECK(c.findPart(99) == nullptr);

        c.findPart(2)->setMass(77.0);                              // правка через неконстантный findPart
        CHECK(near(c.findGroup(20)->parts()[0].mass(), 77.0));
        CHECK(near(c.findGroup(10)->parts()[0].mass(), 1.0));      // соседняя группа не тронута
    }

    section("PartCatalog: isValid");
    {
        PartCatalog c;
        CHECK(c.isValid());                                        // пустой каталог валиден
        c.addGroup(makeGroup(1, "A", {1}));
        CHECK(c.isValid());
        c.findPart(1)->setMass(0);                                 // тип стал невалидным
        CHECK(!c.isValid());
    }

    section("PartCatalog: запись и чтение, порядок сохраняется");
    {
        PartCatalog src;
        src.addGroup(makeGroup(2, "Трансмиссия", {5, 1}));
        src.addGroup(makeGroup(1, "Двигатель"));
        const QByteArray xml = toXml(src);

        QXmlStreamReader r(xml);
        r.readNextStartElement();
        PartCatalog dst;
        dst.loadXml(r);
        CHECK(!r.hasError());
        CHECK(dst.groups().size() == 2);
        CHECK(dst.groups().size() == 2 && dst.groups()[0].id() == 2 && dst.groups()[1].id() == 1);
        CHECK(dst.groups().size() == 2 && dst.groups()[0].parts().size() == 2 && dst.groups()[0].parts()[0].id() == 5);
        CHECK(dst.isValid());
        CHECK(r.isEndElement());
    }

    section("PartCatalog: пустой каталог, неизвестный элемент");
    {
        PartCatalog c;
        CHECK(tryLoad(R"(<catalog/>)", c));
        CHECK(c.groups().isEmpty());
        CHECK(tryLoad(R"(<catalog><note>x</note><group id="1" name="A"/></catalog>)", c));
        CHECK(c.groups().size() == 1);
    }

    section("PartCatalog: плохой ввод, каталог не меняется");
    {
        struct Case { const char *name; QByteArray xml; };
        const Case cases[] = {
            {"дубликат id групп",
             QByteArray(R"(<catalog><group id="1" name="A"/><group id="1" name="B"/></catalog>)")},
            {"дубликат id типа МЕЖДУ группами",
             QByteArray(R"(<catalog><group id="1" name="A">)") + PART_XML(1, "a") + R"(</group><group id="2" name="B">)" + PART_XML(1, "b") + "</group></catalog>"},
            {"невалидный тип внутри",
             QByteArray(R"(<catalog><group id="1" name="A"><part id="1" name="X" mass="0" price="1"><dimensions length="1" width="1" height="1"/></part></group></catalog>)")},
            {"обрезанный XML",
             QByteArray(R"(<catalog><group id="1" name="A"/>)")},
        };
        for (const Case &c : cases) {
            PartCatalog cat;
            cat.addGroup(makeGroup(9, "OLD", {7}));
            QString err;
            qInfo("  - %s", c.name);
            CHECK(!tryLoad(c.xml.constData(), cat, &err));
            qInfo().noquote() << "    Ошибка:" << err;
            CHECK(cat.groups().size() == 1 && cat.groups()[0].id() == 9 && cat.findPart(7) != nullptr);
        }
    }

    section("PartCatalog: контракт, после каталога читается следующий элемент");
    {
        QXmlStreamReader r{QByteArray(R"(<root><catalog><group id="1" name="A"/></catalog><after/></root>)")};
        r.readNextStartElement();                       // <root>
        r.readNextStartElement();                       // <catalog>
        PartCatalog c;
        c.loadXml(r);
        CHECK(!r.hasError());
        CHECK(r.readNextStartElement() && r.name() == QLatin1String("after"));
    }
}

// ---------------------------------------------------------------- Warehouse

static StockItem makeStock(int partId, int quantity)
{
    return StockItem(partId, quantity, 1.0, QDate(2018, 1, 12));
}

static void testWarehouse()
{
    section("Warehouse: addStock / updateStock / removeStock / findStock");
    {
        Warehouse w(1, ru("Лебедушка"));
        CHECK(w.stock().isEmpty() && w.isValid());                 // пустой склад допустим
        CHECK(w.addStock(makeStock(3, 12)));
        CHECK(w.addStock(makeStock(1, 5)));
        CHECK(!w.addStock(makeStock(3, 99)));                      // такой partId уже есть
        CHECK(!w.addStock(StockItem()));                           // невалидная позиция
        CHECK(w.stock().size() == 2 && w.stock()[0].partId() == 3);   // порядок добавления

        CHECK(w.findStock(3) != nullptr && w.findStock(3)->quantity() == 12);
        CHECK(w.findStock(99) == nullptr);

        CHECK(w.updateStock(StockItem(3, 40, 2.0, QDate(2019, 2, 2))));
        CHECK(w.findStock(3)->quantity() == 40 && w.findStock(3)->deliveryDate() == QDate(2019, 2, 2));
        CHECK(!w.updateStock(makeStock(99, 1)));                   // нет такой позиции
        CHECK(!w.updateStock(StockItem(3, -1, 1.0, QDate(2019, 2, 2))));   // невалидная
        CHECK(w.findStock(3)->quantity() == 40);                   // не изменилась

        CHECK(w.removeStock(3));
        CHECK(!w.removeStock(3));
        CHECK(w.stock().size() == 1);
    }

    section("Warehouse: isValid");
    {
        CHECK(!Warehouse(0, "X").isValid());
        CHECK(!Warehouse(1, "").isValid());
    }

    section("Warehouse: запись и чтение, пустой склад");
    {
        Warehouse src(2, ru("4x4 Profi"));
        src.addStock(makeStock(1, 32));
        src.addStock(makeStock(2, 3));
        const QByteArray xml = toXml(src);
        qInfo().noquote() << "XML:" << QString::fromUtf8(xml);

        QXmlStreamReader r(xml);
        r.readNextStartElement();
        Warehouse dst;
        dst.loadXml(r);
        CHECK(!r.hasError());
        CHECK(dst.id() == 2 && dst.name() == ru("4x4 Profi"));
        CHECK(dst.stock().size() == 2 && dst.stock()[0].partId() == 1 && dst.stock()[0].quantity() == 32);
        CHECK(r.isEndElement());

        Warehouse empty;
        CHECK(tryLoad(R"(<warehouse id="1" name="Пустой"/>)", empty));
        CHECK(empty.stock().isEmpty());
    }

    section("Warehouse: плохой ввод, объект не меняется");
    {
        struct Case { const char *name; const char *xml; };
        const Case cases[] = {
            {"дубликат partId",
             R"(<warehouse id="1" name="W"><stock partId="1" quantity="1" price="1" deliveryDate="2018-01-12"/><stock partId="1" quantity="2" price="1" deliveryDate="2018-01-12"/></warehouse>)"},
            {"невалидная позиция",
             R"(<warehouse id="1" name="W"><stock partId="1" quantity="-1" price="1" deliveryDate="2018-01-12"/></warehouse>)"},
            {"обрезанный XML",
             R"(<warehouse id="1" name="W"><stock partId="1" quantity="1" price="1" deliveryDate="2018-01-12"/>)"},
            {"id = 0",
             R"(<warehouse id="0" name="W"/>)"},
            {"пустое имя",
             R"(<warehouse id="1" name=""/>)"},
        };
        for (const Case &c : cases) {
            Warehouse w(9, "OLD");
            w.addStock(makeStock(7, 1));
            QString err;
            qInfo("  - %s", c.name);
            CHECK(!tryLoad(c.xml, w, &err));
            qInfo().noquote() << "    Ошибка:" << err;
            CHECK(w.id() == 9 && w.name() == "OLD" && w.stock().size() == 1 && w.stock()[0].partId() == 7);
        }
    }
}

// ---------------------------------------------------------------- Product

static void testProduct()
{
    section("Product: addComponent / setQuantity / removeComponent");
    {
        Product p(1, "ВАЗ 2121");
        CHECK(p.components().isEmpty() && p.isValid());
        CHECK(p.addComponent(3, 1));
        CHECK(p.addComponent(2, 4));
        CHECK(!p.addComponent(3, 9));                              // тип уже в составе
        CHECK(!p.addComponent(5, 0));                              // количество 0
        CHECK(!p.addComponent(0, 1));                              // partId 0
        CHECK(p.components().size() == 2);
        CHECK(p.hasPart(3) && !p.hasPart(99));
        CHECK(p.quantityOf(2) == 4 && p.quantityOf(99) == 0);

        CHECK(p.setQuantity(2, 7) && p.quantityOf(2) == 7);
        CHECK(!p.setQuantity(99, 1));                              // нет такого
        CHECK(!p.setQuantity(2, 0) && p.quantityOf(2) == 7);       // не изменилось

        CHECK(p.removeComponent(2));
        CHECK(!p.removeComponent(2));
        CHECK(p.components().size() == 1);
    }

    section("Product: isValid");
    {
        CHECK(!Product(0, "X").isValid());
        CHECK(!Product(1, "").isValid());
    }

    section("Product: запись и чтение");
    {
        Product src(1, ru("ВАЗ 2121"));
        src.addComponent(3, 1);
        src.addComponent(2, 2);
        const QByteArray xml = toXml(src);
        qInfo().noquote() << "XML:" << QString::fromUtf8(xml);

        QXmlStreamReader r(xml);
        r.readNextStartElement();
        Product dst;
        dst.loadXml(r);
        CHECK(!r.hasError());
        CHECK(dst.id() == 1 && dst.name() == ru("ВАЗ 2121"));
        CHECK(dst.quantityOf(3) == 1 && dst.quantityOf(2) == 2);
        CHECK(r.isEndElement());

        Product empty;
        CHECK(tryLoad(R"(<product id="1" name="Пустое"/>)", empty));
        CHECK(empty.components().isEmpty());
    }

    section("Product: плохой ввод, объект не меняется");
    {
        struct Case { const char *name; const char *xml; };
        const Case cases[] = {
            {"дубликат partId",
             R"(<product id="1" name="P"><component partId="1" quantity="1"/><component partId="1" quantity="2"/></product>)"},
            {"количество 0",
             R"(<product id="1" name="P"><component partId="1" quantity="0"/></product>)"},
            {"отрицательное количество",
             R"(<product id="1" name="P"><component partId="1" quantity="-3"/></product>)"},
            {"нет quantity",
             R"(<product id="1" name="P"><component partId="1"/></product>)"},
            {"partId = 0",
             R"(<product id="1" name="P"><component partId="0" quantity="1"/></product>)"},
            {"обрезанный XML",
             R"(<product id="1" name="P"><component partId="1" quantity="1"/>)"},
        };
        for (const Case &c : cases) {
            Product p(9, "OLD");
            p.addComponent(7, 3);
            QString err;
            qInfo("  - %s", c.name);
            CHECK(!tryLoad(c.xml, p, &err));
            qInfo().noquote() << "    Ошибка:" << err;
            CHECK(p.id() == 9 && p.components().size() == 1 && p.quantityOf(7) == 3);
        }
    }

    section("Product: контракт, следующее изделие читается после предыдущего");
    {
        QXmlStreamReader r{QByteArray(
            R"(<products><product id="1" name="A"><component partId="1" quantity="1"/></product>)"
            R"(<product id="2" name="B"><component partId="2" quantity="5"/></product></products>)")};
        r.readNextStartElement();                       // <products>
        QList<Product> list;
        while (r.readNextStartElement()) {
            Product p;
            p.loadXml(r);
            if (r.hasError()) break;
            list.append(p);
        }
        CHECK(!r.hasError());
        CHECK(list.size() == 2 && list[1].quantityOf(2) == 5);
    }
}

// ---------------------------------------------------------------- SupplySystem

static QString writeFile(const QTemporaryDir &dir, const char *name, const QByteArray &content)
{
    const QString path = dir.filePath(name);
    QFile f(path);
    f.open(QIODevice::WriteOnly);
    f.write(content);
    return path;
}

static QByteArray readFile(const QString &path)
{
    QFile f(path);
    f.open(QIODevice::ReadOnly);
    return f.readAll();
}

// Небольшая корректная система: тип 1, склад со 2 шт., изделие с 1 шт.
static const char *kMiniSystem =
    R"(<supplySystem id="1">)"
    R"(<catalog><group id="1" name="G">)" PART_XML(1, "a") R"(</group></catalog>)"
    R"(<warehouses><warehouse id="1" name="W"><stock partId="1" quantity="2" price="3.00" deliveryDate="2018-01-12"/></warehouse></warehouses>)"
    R"(<products><product id="1" name="P"><component partId="1" quantity="1"/></product></products>)"
    R"(</supplySystem>)";

static void testSupplySystem()
{
    section("SupplySystem: загрузка реального примера data/supply_system.xml");
    SupplySystem sample;
    {
        QString err;
        CHECK(sample.loadFromFile(SAMPLE_XML, &err));
        qInfo().noquote() << "Ошибка:" << err;
        CHECK(sample.id() == 1);
        CHECK(sample.catalog().groups().size() == 2);
        CHECK(sample.warehouses().size() == 2 && sample.products().size() == 2);
        CHECK(sample.validate().isEmpty() && sample.isValid());
        CHECK(sample.catalog().nextPartId() == 6 && sample.nextWarehouseId() == 3 && sample.nextProductId() == 3);
    }

    section("SupplySystem: запросы главного экрана (макет 1)");
    {
        // изделие 1 = ВАЗ 2121: типы 2, 3 (Трансмиссия), 4 (Двигатель)
        const QList<const PartGroup *> groups = sample.groupsOfProduct(1);
        CHECK(groups.size() == 2);
        CHECK(groups.size() == 2 && groups[0]->name() == ru("Двигатель") && groups[1]->name() == ru("Трансмиссия"));

        const QList<const SparePartType *> parts = sample.partsOfProduct(1, 2);
        CHECK(parts.size() == 2 && parts[0]->id() == 2 && parts[1]->id() == 3);
        CHECK(sample.partsOfProduct(1, 1).size() == 1);            // Двигатель: только тип 4
        CHECK(sample.partsOfProduct(99, 2).isEmpty() && sample.partsOfProduct(1, 99).isEmpty());
        CHECK(sample.groupsOfProduct(99).isEmpty());

        const QList<SupplySystem::StockLocation> loc = sample.stockLocations(3);   // РПМ в сборе 2121
        CHECK(loc.size() == 2);
        CHECK(loc.size() == 2 && loc[0].warehouseName == ru("Лебедушка") && loc[0].item.quantity() == 12
              && loc[0].item.deliveryDate() == QDate(2018, 1, 12));
        CHECK(loc.size() == 2 && loc[1].warehouseName == "4x4 Profi" && loc[1].item.quantity() == 2);
        CHECK(sample.stockLocations(99).isEmpty());
    }

    section("SupplySystem: запись в файл и повторное чтение дают тот же результат");
    {
        QTemporaryDir dir;
        const QString p1 = dir.filePath("out1.xml");
        const QString p2 = dir.filePath("out2.xml");
        QString err;
        CHECK(sample.saveToFile(p1, &err));

        SupplySystem again;
        CHECK(again.loadFromFile(p1, &err));
        CHECK(again.saveToFile(p2, &err));
        CHECK(!readFile(p1).isEmpty() && readFile(p1) == readFile(p2));
        CHECK(readFile(p1).startsWith("<?xml"));
        CHECK(again.catalog().groups().size() == 2 && again.warehouses().size() == 2 && again.products().size() == 2);
        CHECK(again.findProduct(1) != nullptr && again.findProduct(1)->name() == ru("ВАЗ 2121"));
    }

    section("SupplySystem: удаление типа ЗЧ запрещено, пока он используется");
    {
        SupplySystem s;
        CHECK(s.loadFromFile(SAMPLE_XML));
        CHECK(s.isPartUsed(5));                                    // есть на складе 1 и в изделии 2
        CHECK(!s.removePart(5));
        CHECK(s.removeStock(1, 5));
        CHECK(!s.removePart(5));                                   // всё ещё в изделии 2
        CHECK(s.removeComponent(2, 5));
        CHECK(!s.isPartUsed(5));
        CHECK(s.removePart(5));
        CHECK(s.findPart(5) == nullptr && s.isValid());
    }

    section("SupplySystem: ссылки только на существующие типы");
    {
        SupplySystem s;
        CHECK(s.addGroup(PartGroup(1, "G")));
        CHECK(s.addPart(1, makePart(1, "a")));

        Warehouse w(1, "W");
        w.addStock(makeStock(1, 2));
        Warehouse bad(2, "Bad");
        bad.addStock(makeStock(99, 2));                            // 99 нет в каталоге
        CHECK(s.addWarehouse(w));
        CHECK(!s.addWarehouse(bad));
        CHECK(!s.addWarehouse(w));                                 // дубликат id
        CHECK(!s.addStock(1, makeStock(99, 1)));
        CHECK(s.addPart(1, makePart(2, "b")) && s.addStock(1, makeStock(2, 1)));
        CHECK(s.updateStock(1, makeStock(2, 8)) && s.findWarehouse(1)->findStock(2)->quantity() == 8);
        CHECK(!s.updateStock(9, makeStock(2, 8)));                 // нет склада
        CHECK(s.removeStock(1, 2));

        Product p(1, "P");
        p.addComponent(1, 1);
        Product badP(2, "BadP");
        badP.addComponent(99, 1);
        CHECK(s.addProduct(p));
        CHECK(!s.addProduct(badP));
        CHECK(!s.addProduct(p));                                   // дубликат id
        CHECK(!s.addComponent(1, 99, 1));                          // тип не существует
        CHECK(s.addComponent(1, 2, 3));
        CHECK(!s.addComponent(1, 2, 5));                           // уже в составе
        CHECK(s.setComponentQuantity(1, 2, 5) && s.findProduct(1)->quantityOf(2) == 5);
        CHECK(s.removeComponent(1, 2));
        CHECK(!s.removeComponent(9, 2));                           // нет изделия
        CHECK(s.isValid());
        CHECK(s.nextWarehouseId() == 2 && s.nextProductId() == 2);
        CHECK(s.removeWarehouse(1) && !s.removeWarehouse(1));
        CHECK(s.removeProduct(1) && !s.removeProduct(1));
        CHECK(s.removeGroup(1) == false);                          // в группе ещё есть типы
    }

    section("SupplySystem: порядок секций в файле не важен");
    {
        SupplySystem s;
        CHECK(tryLoad(
            R"(<supplySystem id="1">)"
            R"(<products><product id="1" name="P"><component partId="1" quantity="1"/></product></products>)"
            R"(<warehouses><warehouse id="1" name="W"><stock partId="1" quantity="2" price="3.00" deliveryDate="2018-01-12"/></warehouse></warehouses>)"
            R"(<catalog><group id="1" name="G">)" PART_XML(1, "a") R"(</group></catalog>)"
            R"(</supplySystem>)", s));
        CHECK(s.isValid() && s.products().size() == 1);
    }

    section("SupplySystem: плохие файлы, система не меняется");
    {
        QTemporaryDir dir;
        struct Case { const char *name; QByteArray content; };
        const Case cases[] = {
            {"ссылка на несуществующий тип (склад)",
             QByteArray(R"(<supplySystem id="1"><catalog/><warehouses><warehouse id="1" name="W"><stock partId="99" quantity="1" price="1" deliveryDate="2018-01-12"/></warehouse></warehouses></supplySystem>)")},
            {"ссылка на несуществующий тип (изделие)",
             QByteArray(R"(<supplySystem id="1"><catalog/><products><product id="1" name="P"><component partId="99" quantity="1"/></product></products></supplySystem>)")},
            {"дубликат id складов",
             QByteArray(R"(<supplySystem id="1"><warehouses><warehouse id="1" name="A"/><warehouse id="1" name="B"/></warehouses></supplySystem>)")},
            {"дубликат id изделий",
             QByteArray(R"(<supplySystem id="1"><products><product id="1" name="A"/><product id="1" name="B"/></products></supplySystem>)")},
            {"два каталога",
             QByteArray(R"(<supplySystem id="1"><catalog/><catalog/></supplySystem>)")},
            {"id системы = 0",
             QByteArray(R"(<supplySystem id="0"/>)")},
            {"другой корневой элемент",
             QByteArray(R"(<other id="1"/>)")},
            {"это не XML",
             QByteArray("просто текст")},
            {"обрезанный файл",
             QByteArray(R"(<supplySystem id="1"><catalog/>)")},
            {"пустой файл",
             QByteArray("")},
        };
        int n = 0;
        for (const Case &c : cases) {
            SupplySystem s;
            s.addGroup(PartGroup(5, "OLD"));
            const QString path = writeFile(dir, qPrintable(QString("bad%1.xml").arg(n++)), c.content);
            QString err;
            qInfo("  - %s", c.name);
            CHECK(!s.loadFromFile(path, &err));
            qInfo().noquote() << "    Ошибка:" << err;
            CHECK(!err.isEmpty());
            CHECK(s.catalog().groups().size() == 1 && s.catalog().groups()[0].id() == 5);
        }

        SupplySystem s;
        QString err;
        CHECK(!s.loadFromFile(dir.filePath("нет_такого_файла.xml"), &err));
        qInfo().noquote() << "Ошибка:" << err;
        CHECK(!err.isEmpty());
    }

    section("SupplySystem: пустая система и миникорректный файл");
    {
        SupplySystem s;
        CHECK(tryLoad(R"(<supplySystem id="7"/>)", s));
        CHECK(s.id() == 7 && s.isValid() && s.warehouses().isEmpty());

        QTemporaryDir dir;
        SupplySystem m;
        CHECK(m.loadFromFile(writeFile(dir, "mini.xml", QByteArray(kMiniSystem))));
        CHECK(m.isValid() && m.stockLocations(1).size() == 1);
    }
}

// ---------------------------------------------------------------- main

int main()
{
    testXmlUtil();
    testNamedEntity();
    testDimensions();
    testStockItem();
    testSparePartType();
    testPartGroup();
    testPartCatalog();
    testWarehouse();
    testProduct();
    testSupplySystem();

    qInfo("\nИтого провалено проверок: %d", failures);
    return failures == 0 ? 0 : 1;
}