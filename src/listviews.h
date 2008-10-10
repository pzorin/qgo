#include <QtGui>
class GameListing;
class PlayerListing;

#define LESSTHAN	-1
#define EQUALTO		0
#define GREATERTHAN	1
/* ListItem can't have pure virtual functions because
 * we do actually use it for the header items */
class ListItem
{
	public:
		ListItem() {};
		ListItem(const QList <QVariant> & data);
		virtual ~ListItem() {};
		virtual QVariant data(int column) const;
		virtual int columnCount() const;
		virtual int compare(const ListItem &, int) const { return 0;};
	protected:
		int compareNames(const QString & name1, const QString & name2) const;
	private:
		QList <QVariant> itemData;
};

enum ObserverListingColumn { OC_NAME=0, OC_RANK, O_TOTALCOLUMNS };

enum PlayerListingColumn { PC_STATUS=0, PC_NAME, PC_RANK, PC_PLAYING,
				PC_OBSERVING, PC_WINS, PC_LOSSES, PC_IDLE, PC_COUNTRY,
				PC_MATCHPREFS, P_TOTALCOLUMNS};

enum GameListingColumn { GC_ID=0, GC_WHITENAME, GC_WHITERANK, GC_BLACKNAME,
				GC_BLACKRANK,
				GC_MOVES, GC_SIZE, GC_HANDICAP, GC_KOMI,
				GC_BYOMI, GC_FLAGS,
				GC_OBSERVERS, G_TOTALCOLUMNS };

class ObserverListItem : public ListItem
{
	public:
		ObserverListItem(const PlayerListing * l);	
		virtual QVariant data(int column) const;
		int columnCount() const;
		const PlayerListing * getListing(void) const { return listing; };
		virtual int compare(const ListItem &, int column) const;
	private:
		const PlayerListing * listing;
};

class GamesListItem : public ListItem
{
	public:
		GamesListItem(const GameListing * l);
		virtual QVariant data(int column) const;
		int columnCount() const;
		const GameListing * getListing(void) const { return listing; };
		virtual int compare(const ListItem &, int column) const;
	private:
		const GameListing * listing;
};

class PlayerListItem : public ListItem
{
	public:
		PlayerListItem(const PlayerListing * l);
		virtual QVariant data(int column) const;
		int columnCount() const;
		const PlayerListing * getListing(void) const { return listing; };
		virtual int compare(const ListItem &, int column) const;
	private:
		const PlayerListing * listing;
};

class ListModel : public QAbstractItemModel	//QAbstractItemModel??
{
	 public:
	 	ListModel();
		~ListModel();
		void insertListing(const ListItem & item);
		//void updateListing(const ListItem & item);
		//void removeListing(const ListItem & item);
		int priorityCompare(const ListItem & i, const ListItem & j);
		QModelIndex parent(const QModelIndex & index) const;
		//bool hasChildren(const QModelIndex & parent) const;
		int rowCount(const QModelIndex & parent) const;
		int columnCount(const QModelIndex & parent) const;
		//bool hasIndex(int row, int column, const QModelIndex & parent) const;
		QModelIndex index(int row, int column, const QModelIndex & parent) const;
		virtual QVariant data(const QModelIndex & index, int role) const;
		Qt::ItemFlags flags(const QModelIndex & index) const;
		QVariant headerData(int section, Qt::Orientation orientation, int role) const;
		virtual void sort(int column, Qt::SortOrder order = Qt::AscendingOrder);
	protected:
		ListItem * headerItem;
		QList <const ListItem *> items;	//try it with the one list
		QList <int> sort_priority;
		Qt::SortOrder list_sort_order;
	private:
		void quicksort(int b, int e);
		int qs_partition(int b, int e);
};

class ObserverListModel : public ListModel
{
	public:
		ObserverListModel();
		~ObserverListModel();
		void insertListing(const PlayerListing * l);
		void removeListing(const PlayerListing * l);
		void clearList(void);
		virtual QVariant data(const QModelIndex & index, int role) const;
		void setAccountName(QString name) { account_name = name; };
	private:
		QString account_name;
};

class PlayerListModel : public ListModel
{
	public:
		PlayerListModel();
		~PlayerListModel();
		void insertListing(const PlayerListing * l);
		void updateListing(const PlayerListing * l);
		void removeListing(const PlayerListing * l);
		void clearList(void);
		virtual QVariant data(const QModelIndex & index, int role) const;
		const PlayerListing * playerListingFromIndex(const QModelIndex &);
		void setAccountName(QString name) { account_name = name; };
	private:
		QString account_name;
};

class GamesListModel : public ListModel
{
	public:
		GamesListModel();
		~GamesListModel();
		void insertListing(const GameListing * l);
		void updateListing(const GameListing * l);
		void removeListing(const GameListing * l);
		void clearList(void);
		virtual QVariant data(const QModelIndex & index, int role) const;
		const GameListing * gameListingFromIndex(const QModelIndex &);
};

class PlayerSortProxy : public QSortFilterProxyModel
{
	/* IGS ranks probably up to 10000, but ORO would need 100000 so... */
	public:
		PlayerSortProxy() : rankMin(0), rankMax(100000), openOnly(0) {};
		virtual void sort(int column, Qt::SortOrder order = Qt::AscendingOrder);
		bool filterAcceptsRow(int sourceRow, const QModelIndex & sourceParent) const;
		void setFilter(int rn, int rm);
		void setFilter(bool oo);
	private:
		int rankMin;
		int rankMax;
		bool openOnly;
};

extern std::map<class PlayerListing *, unsigned short> removed_player;
